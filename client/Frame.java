package com.gooroos.FrameGrabber.client;

public class Frame {
	public byte[] pixels = null;
	public long time = 0L;
	public int ptrX = -1;
	public int ptrY = -1;
	
	public Frame(int width, int height)
	{
		pixels = new byte[width * height *4];
	}
}
