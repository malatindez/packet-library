#pragma once

#include "packet.hpp"

namespace mal_packet_weaver
{
    template <typename T>
    struct PacketTypeRegistrationHelperNoexcept;
    /**
     * @brief A class responsible for registering and creating packet deserializers.
     */
    class PacketFactory
    {
    public:
        /**
         * @brief Register a packet deserializer function for a specific PacketType.
         *
         * This template function allows registration of deserializer functions for packet types
         * that satisfy the IsPacket concept. The function associates the packet's static_type
         * with its deserialize member function.
         *
         * @tparam PacketType The packet type that satisfies the IsPacket concept.
         */
        template <IsPacket PacketType>
        static inline void RegisterDeserializer()
        {
            if (auto it = instance().packet_deserializers_.find(PacketType::static_type);
                it != instance().packet_deserializers_.end())
            {
                std::unique_ptr<Packet> (*const *ptr)(const ByteView) = it->second.target<std::unique_ptr<Packet> (*)(const ByteView)>();
                if (ptr && ptr == &(& PacketType::deserialize))  // Same target
                {
                    return;
                }
                throw std::invalid_argument("Packet deserializer already initialized with different function!");
            }
            instance().packet_deserializers_[PacketType::static_type] = PacketType::deserialize;
        }

        /**
         * @brief Register a packet deserializer function for a specific packet ID.
         *
         * This function allows registration of a custom packet deserializer function for a
         * unique packet ID. It associates the given packet_id with the provided factory function.
         *
         * @param packet_id The unique packet ID.
         * @param factory The packet deserialization factory function.
         */
        static void RegisterDeserializer(UniquePacketID packet_id, PacketDeserializeFunc factory)
        {
            if (auto it = instance().packet_deserializers_.find(packet_id);
                it != instance().packet_deserializers_.end())
            {
                std::unique_ptr<Packet> (*const *ptr)(const ByteView) = factory.target<std::unique_ptr<Packet> (*)(const ByteView)>();
                if (!ptr || ptr == it->second.target<std::unique_ptr<Packet> (*)(const ByteView)>())
                {
                    throw std::invalid_argument("Packet deserializer already initialized with different function!");
                }
                return;
            }
            instance().packet_deserializers_[packet_id] = factory;
        }

        /**
         * @brief Deserialize a byte view into a unique pointer of the specified packet type.
         *
         * This function uses the registered packet deserializer functions to deserialize the
         * binary data from a ByteView and reconstruct a unique pointer to a packet instance.
         *
         * @param bytearray The ByteView containing the binary serialized data.
         * @param packet_type The unique packet ID specifying the packet type.
         * @return A unique pointer to the deserialized packet instance.
         */
        [[nodiscard]] static inline std::unique_ptr<Packet> Deserialize(const ByteView &bytearray,
                                                                        UniquePacketID packet_type)
        {
            auto it = instance().packet_deserializers_.find(packet_type);
            if (it != instance().packet_deserializers_.end())
            {
                return it->second(bytearray);
            }
            // TODO: MAL_PACKET_WEAVER_VERBOSE_LEVEL, output to spdlog if there's no deserializer.
            return nullptr;
        }
        /**
        * @brief Instance of the Packet Factory.
        */
        static PacketFactory& instance()
        {
            if (instance_ == nullptr)
            {
                instance_ = std::unique_ptr<PacketFactory>(new PacketFactory);
            }
            return *instance_;
        }

    private:
        PacketFactory() = default;
        static std::unique_ptr<PacketFactory> instance_;
        /**
         * @brief Map storing registered packet deserializer functions.
         */
        std::unordered_map<UniquePacketID, PacketDeserializeFunc> packet_deserializers_;
    };

    /**
     * @brief Helper class for registering a packet type with the PacketFactory.
     *
     * This class is used to register a specific packet type with the PacketFactory
     * during static initialization. It ensures that the packet type is registered
     * with the PacketFactory before the main function is called.
     *
     * @tparam T The packet type to be registered.
     */
    template <typename T>
    struct PacketTypeRegistrationHelper
    {
        /**
         * @brief Constructor. Registers the packet type with the PacketFactory.
         */
        PacketTypeRegistrationHelper()
        {
            PacketFactory::RegisterDeserializer<T>();
        }
    };

}  // namespace mal_packet_weaver
