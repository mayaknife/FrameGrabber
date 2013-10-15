package gooroos.framegrabber.client;

import java.util.concurrent.ArrayBlockingQueue;

public class FrameQueue extends ArrayBlockingQueue<Frame>
{
	public FrameQueue(int capacity)
	{
		super(capacity);
	}
	
	public void put(Frame frame)
	{
		try {
			super.put(frame);
		} catch (InterruptedException e) {
		}
	}
	
	public Frame take()
	{
		Frame frame = null;
		
		try {
			frame = super.take();
		} catch (InterruptedException e) {
		}
		
		return frame;
	}
}
