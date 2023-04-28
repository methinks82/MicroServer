# MicroServer
> **Microserver is currently not being maintained** as there are several other libraries with similar functionality. This library should be used for educational purposes only

Microserver (Also called Alex's MicroServer or ASM) is a library to provide support for a simple and lightweight server that can includes webserver and websockets, but which can be expanded to handle any kind of protocols. For simplicity it can be used as a single-header file.

## Usage
Create a new server using the following steps:

1. **Create a server**  
Simply create new object of either the the asm::Server class or the asm::ThreadedServer class if you need a non-blocking server
``` cpp
ams::Server server; // Blocking server
ams::ThreadedServer server // Non-blocking server
```

2. **Create and register any protocols that your server should accept**  
AMS comes with built-in support for HTTP and Websocket protocols. Other protocols can be easily added

    Create an instance of the protocol's class, then add it to the server
``` cpp
ams::HttpProtocol http; // enable an object to listen for the http protocol
server.addProtocol http; // add the listener to the server
```
3. **Configure the protocol**  
Part of the flexibility of this library comes from the ability to customize how a given protocol deals with specific events. To accomplish this, some protocols may allow you to define custom functions. One example of this can be seen in the WebSocket protocol:
``` cpp
// provide the websocket with a custom function to call when a new connection is made
websocket.setOnConnect(
    []( // for this example we'll declare the new function as a lambda
        ams::ProtocolBase* protocol, // give us access to the protocol's functions
        ams::Connection& connection // information about the new connection
        )
        {
            // let all the other connected users know there is a new user    
            std::string msg = std::to_string(connection.sock) + " has joined the server";
            protocol->broadcast ( msg );
        }
    );
```
The functions that can be added depend on the protocol. For example websockets support custom functions for:
- onConnect
- onRecieve
- onDisconnect

> To see an example of a server application take a look at [ExampleApp.cpp](https://github.com/methinks82/MicroServer/blob/main/example/ExampleApp.cpp) which creates a webserver capable of serving files, connecting and disconnecting, and even hosting a simple chat program.