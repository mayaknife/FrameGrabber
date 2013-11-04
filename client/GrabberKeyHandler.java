package com.gooroos.framegrabber.client;

import java.util.EnumSet;

import net.minecraft.client.Minecraft;
import net.minecraft.client.settings.KeyBinding;
import cpw.mods.fml.client.FMLClientHandler;
import cpw.mods.fml.client.registry.KeyBindingRegistry.KeyHandler;
import cpw.mods.fml.common.TickType;

public class GrabberKeyHandler  extends KeyHandler
{
    private EnumSet tickTypes = EnumSet.of(TickType.CLIENT);
    private ClientTickHandler tickHandler;
   
    public GrabberKeyHandler(KeyBinding[] keyBindings, boolean[] repeatings, ClientTickHandler tickHandler)
    {
            super(keyBindings, repeatings);
            this.tickHandler = tickHandler;
    }
    
    @Override
    public String getLabel()
    {
            return "FrameGrabberKey";
    }
    
    @Override
    public void keyDown(EnumSet<TickType> types, KeyBinding kb, boolean tickEnd, boolean isRepeat)
    {
    }
    
    @Override
    public void keyUp(EnumSet<TickType> types, KeyBinding kb, boolean tickEnd)
    {
    	if (tickEnd) {
    		Minecraft mc = FMLClientHandler.instance().getClient();
    		
    		//	Let's make sure that we're not triggering on keystrokes being entered into a GUI
    		//	(e.g. when the player is typing text into a sign).
    		//
    		if (mc.inGameHasFocus && (mc.currentScreen == null)) {
    			mc.displayGuiScreen(new ConfirmRecordingDialog(tickHandler));
    		}
    	}
    }
    
    @Override
    public EnumSet<TickType> ticks()
    {
            return tickTypes;
    }
}
