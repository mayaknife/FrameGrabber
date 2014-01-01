package com.gooroos.framegrabber.client;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;

public class FrameWriter extends Thread {
	public boolean initSucceeded = false;
	
	private int fileFormatVersion = 2;
	private FrameQueue waiting = null;
	private FrameQueue written = null;
	private int width = 0;
	private int height = 0;
	private File file = null;
	private DataOutputStream outputStream = null;
	
	public FrameWriter(File file, int width, int height, FrameQueue incoming, FrameQueue outgoing)
	{
		this.file = file;
		this.width = width;
		this.height = height;
		this.waiting = incoming;
		this.written = outgoing;
		//Deflater def = new Deflater(Deflater.BEST_SPEED);
		
		try {
			// outputStream = new DataOutputStream(new DeflaterOutputStream(new FileOutputStream(file), def));
			outputStream = new DataOutputStream(new FileOutputStream(file));
			outputStream.writeInt(fileFormatVersion);
			outputStream.writeInt(width);
			outputStream.writeInt(height);
			initSucceeded = true;
		} catch (Exception e) {
			System.out.println("Could not open output file. Stopping recording.");
		}
	}
	
	public void run()
	{
		Frame frame = null;
		int	frameCount = 0;
		
		while (true)
		{
			boolean interrupted = false;
			
			//	Grab the next frame to write from the waiting queue.
			//
			frame = waiting.take();

			//	If the frame has a negative time, that's our signal to quit.
			//
			if (frame.time < 0L) {
				break;
			}
			
			try {
				//	Store the frame time.
				//
				outputStream.writeLong(frame.time);
				
				//	Store the pointer coords.
				//
				outputStream.writeInt(frame.ptrX);
				outputStream.writeInt(frame.ptrY);
				
				//	Store the image.
				//
				outputStream.write(frame.pixels);
				outputStream.flush();
				++frameCount;

			} catch (Exception e) {
				System.out.println("Error writing frame " + frameCount + " to output stream.");
			}
			
			//	Put the frame onto the written queue.
			//
			written.put(frame);
		}
		
		//	Close the output stream and exit.
		//
		try {
			outputStream.close();
		} catch (Exception e) {
		}
		
		System.out.println("Stop recording to '" + file.getAbsolutePath() + "' after " + frameCount + " frames.");
	}
}
