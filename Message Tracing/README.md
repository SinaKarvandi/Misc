# Custom Message Tracing and Buffer Transformation

This project is extending a Microsoft sample for IRP Pending into a Message Tracing and Buffer Sharing from Kernel mode -> Usermode.

![](https://github.com/SinaKarvandi/Misc/raw/master/Imgs/Tracer.gif)

## Why you should use it instead of WPP Tracing?
It's because WPP Tracing needs extra applications to translate the buffer but instead this messaging mechanism is really simple, you don't need to deal with .etl and .tmf files to read the messages of WPP Tracing.

## Design
I used DPC and IRP Based Pending mechanisms in Windows to handle messages, you can send both a raw buffer and a message (`sprintf` like format to the usermode.).
Each core has it's separate pool for THE message, you can insert message to another core's pool.

A core buffer is like this , it's divided into `MaximumPacketsCapacity` chucks,
each chunk has `PacketChunkSize + sizeof(BUFFER_HEADER)` size.
```

			 _________________________
			|      BUFFER_HEADER      |
			|_________________________|
			|			  |
			|           BODY	  |
			|         (Buffer)	  |
			| size = PacketChunkSize  |
			|			  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|			  |
			|           BODY	  |
			|         (Buffer)	  |
			| size = PacketChunkSize  |
			|		       	  |
			|_________________________|
			|		          |
			|			  |
			|			  |
			|			  |
			|	      .           |
			|	      .		  |
			|	      .		  |
			|			  |
			|			  |
			|			  |
			|			  |
			|_________________________|
			|      BUFFER_HEADER      |
			|_________________________|
			|			  |
			|           BODY	  |
			|         (Buffer)	  |
			| size = PacketChunkSize  |
			|			  |
			|_________________________|
			
```

## Configuration
You can change the following lines in order to change the buffer size and change the maximum capacity for each pool.
```
// Default buffer size
#define MaximumPacketsCapacity 1000 // number of packets
#define PacketChunkSize		512
#define UsermodeBufferSize  sizeof(UINT32) + PacketChunkSize + 1 /* Becausee of Opeation code at the start of the buffer + 1 for null-termminating */
#define BufferSize MaximumPacketsCapacity * (PacketChunkSize + sizeof(BUFFER_HEADER))
```
## How to use?

In order to use this event tracing, you can use the following function in you code to send a buffer to the pool.
```
SendBuffer(UINT32 OperationCode, PVOID Buffer, UINT32 BufferLength, UINT32 CoreIndex);
```
The above method gets the `OperationCode`, this code will be sent to the usermode code to describe the intention of this message, send the buffer and also the length of the buffer + an index to specify the core which you want to send your message into its pool.

```
BOOLEAN ReadBuffer(UINT32 CoreIndex, PVOID BufferToSaveMessage, UINT32* ReturnedLength);
```
`ReadBuffer` reads the buffer from a special core pool.
```
BOOLEAN CheckForNewMessage(UINT32 CoreIndex);
```
Checks whether a special core pool has a new message or not.
```
BOOLEAN SendMessageToQueue(UINT32 OperationCode, UINT32 CoreIndex, const char* Fmt, ...)
```
The above functions are used to send `Printf` and `Sprintf` like buffers to the usermode. e.g. as a message tracing for debugging purposes.

In order to send a special message (look at the .gif above), you can change the following line or put this function everywhere you need.
```
// change the following line
SendMessageToQueue(0x85, 0, "Sina's answer is %s [%d] and %s - + 0x%x tttuuii", "Hello", counter, "Second Hello", 0x1234);
// end changes
```
## Run the sample

To test this driver, copy the test application, event.exe, and the driver to the same directory, and run the application. The application will automatically load the driver, if it's not already loaded, and interact with the driver. When you exit the app, the driver will be stopped, unloaded, and removed.

To run the test application, enter the following command in the command window:

`C:\>event.exe <0|1>`

*Only 0 is for transferring buffer from kernel to user, if you use 1 then it's just triggering an event and not passing message buffers.*
The first command-line parameter, `<0|1>`, specify 0 for IRP-based notification and 1 for event-based notification.

License
----

MIT


[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

