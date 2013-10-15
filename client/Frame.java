package gooroos.framegrabber.client;

public class Frame {
	public byte[] pixels = null;
	public long time = 0L;
	
	public Frame(int width, int height)
	{
		pixels = new byte[width * height *4];
	}
}
