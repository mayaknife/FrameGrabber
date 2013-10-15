package com.gooroos.framegrabber.client;

import java.util.EnumSet;

import net.minecraft.client.settings.KeyBinding;
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
    	if (tickEnd) {
            tickHandler.toggleRecording();
    	}
    }
    
    @Override
    public void keyUp(EnumSet<TickType> types, KeyBinding kb, boolean tickEnd)
    {
            //What to do when key is released/up
    }
    
    @Override
    public EnumSet<TickType> ticks()
    {
            return tickTypes;
    }
}
