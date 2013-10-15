package gooroos.framegrabber.client;

import java.io.File;
import java.nio.ByteBuffer;
import java.util.EnumSet;

import org.lwjgl.BufferUtils;
import org.lwjgl.opengl.GL11;
import org.lwjgl.opengl.GL12;

import net.minecraft.client.Minecraft;

import cpw.mods.fml.client.FMLClientHandler;
import cpw.mods.fml.common.ITickHandler;
import cpw.mods.fml.common.TickType;

public class ClientTickHandler implements ITickHandler
{
	private final int kNumBufferedFrames = 3;
	private final File kOutputDir = new File("/deanDrive/deane/Videos/temp");
	
	private ByteBuffer buffer = null;
	private boolean isRecording = false;

	private int height = 0;
	private int width = 0;
	
	private long lastFrameTime = -1;
	private long timePerFrame = 0;
	private double targetFrameRate = 24.0;
	private long numTicks = 0;
	private long numRenderTicks = 0;
	
	private FrameQueue freeFrames = null;
	private FrameQueue framesToBeWritten = null;
	
	private FrameWriter frameWriter = null;
	
	public ClientTickHandler()
	{
		freeFrames = new FrameQueue(3);
		framesToBeWritten = new FrameQueue(3);
	}
	
	@Override
	public void tickStart(EnumSet<TickType> type, Object... tickData) 
	{
		//	If we do the frame grab at tickStart, we're getting the frame from the previous tick render.
		//	It's possible that some other mods have run before us on this tick and done some rendering,
		//	but it's much more likely that they will wait until after MC has done its rendering. So the
		// odds are high that we'll get a complete frame here.
		//
		//	If we do the frame grab at tickEnd, then we're getting the frame from this tick, after MC
		//	has finished its rendering, but possibly before other mods have done theirs. It all depends
		//	upon the order in which mods' tickEnd() are executed. I started out using tickEnd() but
		//	that resulted in Rei's Minimap flashing in the resulting video, which leads me to believe
		//	that the call order can change. By using tickStart() instead, the minimap flashing was
		//	resolved.
		//
		numTicks++;
		
		if(type.contains(TickType.RENDER))
		{
			numRenderTicks++;
			
			//	Don't need this at the moment, but here as a reminder.
			//	The sub-tick time ranges from 0.0 - 1.0. Still not sure
			//	exactly what it represents.
			//
			float renderSubTickTime = (Float)tickData[0];
			
			if (isRecording) {
				long curTime = System.nanoTime();
				
				if (curTime - lastFrameTime >= timePerFrame) {
					recordFrame();
					
					if (lastFrameTime < 0) {
						lastFrameTime = curTime;
					} else {
						lastFrameTime += ((curTime - lastFrameTime) / timePerFrame) * timePerFrame;
					}
				}
			}
		}
	}

	@Override
	public void tickEnd(EnumSet<TickType> type, Object... tickData) 
	{
	}

	@Override
	public EnumSet<TickType> ticks() {
		return EnumSet.of(TickType.RENDER);
	}

	@Override
	public String getLabel() {
		return "FrameGrabber Client";
	}

	public void toggleRecording()
	{
		isRecording = !isRecording;
		
		if (isRecording) {
			
			Minecraft mc = FMLClientHandler.instance().getClient();
			
			width = mc.displayWidth & 0xfffe;
			height = mc.displayHeight & 0xfffe;

			buffer = BufferUtils.createByteBuffer(width * height * 4);
			
			lastFrameTime = -1L;
			timePerFrame = (long)(1000000000.0 / targetFrameRate);
			numTicks = 0;
			numRenderTicks = 0;

			//	If there was an earlier frame writer, make sure that it has exited.
			//
			if (frameWriter != null) {
				try {
					frameWriter.join();
				} catch (InterruptedException e) {
				}
				
				frameWriter = null;
			}

			//	Clear the queues of any old frames, since they might not be
			//	of the right dimensions.
			//
			freeFrames.clear();
			framesToBeWritten.clear();
			
			//	Fill the free Frame queue.
			//
			for (int i = 0; i < kNumBufferedFrames; ++i) {
				freeFrames.put(new Frame(width, height));
			}
			
			//	Find a filename which is not in use.
			//
			File file = null;
			
			try {
				file = File.createTempFile("frameGrabber-", ".out", kOutputDir);
			} catch (Exception e) {
				System.err.println("Could not create output file in '" + kOutputDir.getAbsolutePath() + "'.");
				isRecording = false;
			}

			if (isRecording) {
				//	Start up the writer process.
				//
				frameWriter = new FrameWriter(file, width, height, framesToBeWritten, freeFrames);
				
				if (frameWriter.initSucceeded) {
					System.out.println("Started recording to file '" + file.getAbsolutePath() + "'.");
					frameWriter.start();
				} else {
					System.err.println("Failed to start frameWriter.");
					isRecording = false;
					frameWriter = null;
				}
			}
		} else {
			//	Send the frame writer a frame with a negative time, which will tell it to exit.
			//
			Frame frame = freeFrames.take();
			frame.time = -1L;
			framesToBeWritten.add(frame);
		}
	}
	
	private void recordFrame()
	{
		GL11.glPixelStorei(GL11.GL_PACK_ALIGNMENT, 1);
		GL11.glPixelStorei(GL11.GL_UNPACK_ALIGNMENT, 1);
		buffer.clear();
	
		// BufferedImage.setRGB() expects the pixel components to be
		// packed into a 32-bit int in ARGB order. OpenGL doesn't provide
		// an ARGB format, so instead we use BGRA and specify a pixel type
		// which reverses that ordering (the '_REV' at the end).
		//
		GL11.glReadPixels(
			0, 0, width, height, GL12.GL_BGRA, GL11.GL_UNSIGNED_BYTE, buffer
		);

		//	Get a free frame from the queue.
		//
		Frame frame = freeFrames.take();
		
		//	Set the frame data into the frame.
		//
		buffer.clear();
		buffer.get(frame.pixels);
		frame.time = System.nanoTime();

		//	Put it onto the queue of frames to be written.
		//
		framesToBeWritten.put(frame);
	}
}
