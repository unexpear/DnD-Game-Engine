/**
 * @file network_manager.h
 * @brief Network manager for multiplayer support.
 * 
 * Provides basic client-server networking using UDP/TCP.
 * Supports:
 *   - Client connection to server
 *   - Server hosting
 *   - Reliable and unreliable message sending
 *   - Networked variables (replicated properties)
 * 
 * @author DND Engine Team
 * @date 2026-05-06
 * @version 0.8.0
 */

#pragma once

#include "core/types.h"
#include <string>
#include <functional>
#include <vector>

namespace Dnd {

// ==========================================
// ENUM: NetworkMode
// ==========================================

/**
 * @enum NetworkMode
 * @brief Network operation mode.
 */
enum class NetworkMode : i32 {
    NONE,      ///< Not connected (single player)
    CLIENT,    ///< Connected as client
    SERVER     ///< Running as server
};

// ==========================================
// STRUCT: NetworkMessage
// ==========================================

/**
 * @struct NetworkMessage
 * @brief A network message with data payload.
 */
struct NetworkMessage {
    u32 senderId;          ///< Sender client ID (0 = server)
    u8* data;               ///< Message data
    u32 dataSize;           ///< Data size in bytes
    b8 reliable;            ///< Whether to use reliable delivery

    /// @brief Default constructor.
    NetworkMessage() : senderId(0), data(nullptr), dataSize(0), reliable(false) {}
};

// ==========================================
// CLASS: NetworkManager
// ==========================================

/**
 * @class NetworkManager
 * @brief Singleton network manager for multiplayer.
 * 
 * The NetworkManager handles all network communication.
 * It supports both client and server modes.
 * 
 * Usage (client):
 * @code
 * NetworkManager::Get().Initialize();
 * NetworkManager::Get().ConnectToServer("127.0.0.1", 12345);
 * @endcode
 * 
 * Usage (server):
 * @code
 * NetworkManager::Get().Initialize();
 * NetworkManager::Get().StartServer(12345);
 * @endcode
 */
class NetworkManager {
public:
    /**
     * @brief Gets the singleton instance.
     * @return Reference to the NetworkManager instance.
     */
    static NetworkManager& Get();

    /**
     * @brief Initializes the network system.
     * @return true if successful.
     */
    b8 Initialize();

    /**
     * @brief Shuts down the network system.
     */
    void Shutdown();

    /**
     * @brief Updates network (process incoming/outgoing messages).
     * @param deltaTime Frame delta time.
     */
    void Update(f32 deltaTime);

    // Server methods

    /**
     * @brief Starts a server on the given port.
     * @param port Port to listen on.
     * @return true if server started successfully.
     */
    b8 StartServer(u16 port);

    /**
     * @brief Stops the server.
     */
    void StopServer();

    /**
     * @brief Gets the number of connected clients.
     * @return Client count.
     */
    u32 GetClientCount() const;

    // Client methods

    /**
     * @brief Connects to a server.
     * @param address Server address (IP or hostname).
     * @param port Server port.
     * @return true if connection initiated successfully.
     */
    b8 ConnectToServer(const char* address, u16 port);

    /**
     * @brief Disconnects from the server.
     */
    void Disconnect();

    // Message methods

    /**
     * @brief Sends a message to a specific client (server only).
     * @param clientId Client ID to send to.
     * @param data Message data.
     * @param dataSize Data size in bytes.
     * @param reliable Whether to use reliable delivery.
     */
    void SendToClient(u32 clientId, const void* data, u32 dataSize, b8 reliable = true);

    /**
     * @brief Sends a message to the server (client only).
     * @param data Message data.
     * @param dataSize Data size in bytes.
     * @param reliable Whether to use reliable delivery.
     */
    void SendToServer(const void* data, u32 dataSize, b8 reliable = true);

    /**
     * @brief Sends a message to all clients (server only).
     * @param data Message data.
     * @param dataSize Data size in bytes.
     * @param reliable Whether to use reliable delivery.
     */
    void BroadcastToAll(const void* data, u32 dataSize, b8 reliable = true);

    /**
     * @brief Sets the message handler callback.
     * @param callback Function to call when a message is received.
     */
    void SetMessageHandler(std::function<void(const NetworkMessage&)> callback);

    /// @brief Gets the current network mode.
    NetworkMode GetMode() const { return m_mode; }

    /// @brief Checks if connected (client or server with clients).
    b8 IsConnected() const { return m_mode != NetworkMode::NONE; }

private:
    /**
     * @brief Private constructor (singleton).
     */
    NetworkManager();

    /**
     * @brief Private destructor.
     */
    ~NetworkManager();

    NetworkMode m_mode;                         ///< Current mode
    b8 m_initialized;                           ///< Whether initialized
    std::function<void(const NetworkMessage&)> m_messageHandler; ///< Message callback
};

} // namespace Dnd
