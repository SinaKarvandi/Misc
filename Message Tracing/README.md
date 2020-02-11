# Custom Message Tracing and Buffer Transformation

This project is extending a Microsoft sample for IRP Pending into a Message Tracing and Buffer Sharing from Kernel mode -> Usermode.

![](https://pandao.github.io/editor.md/examples/images/4.jpg)

## Why you should use it instead of WPP Tracing?
It's because WPP Tracing needs extra applications to translate the buffer but instead this messaging mechanism is really simple, you don't need to deal with .etl and .tmf files to read the messages of WPP Tracing.

## Design
I used DPC and IRP Based Pending mechanisms in Windows to handle messages, you can send both a raw buffer and a message (`sprintf` like format to the usermode.).
Each core has it's seprate pool for message, you can insert message to another core's pool.

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


## Run the sample

To test this driver, copy the test application, event.exe, and the driver to the same directory, and run the application. The application will automatically load the driver, if it's not already loaded, and interact with the driver. When you exit the app, the driver will be stopped, unloaded, and removed.

To run the test application, enter the following command in the command window:

`C:\>event.exe <0|1>`

The first command-line parameter, `<0|1>`, specify 0 for IRP-based notification and 1 for event-based notification.

License
----

MIT


[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)
