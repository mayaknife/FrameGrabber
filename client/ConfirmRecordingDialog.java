package com.gooroos.framegrabber.client;

import net.minecraft.client.gui.GuiButton;
import net.minecraft.client.gui.GuiScreen;
import net.minecraft.client.gui.GuiScreenConfirmation;
import net.minecraft.client.mco.GuiScreenConfirmationType;

import cpw.mods.fml.relauncher.Side;
import cpw.mods.fml.relauncher.SideOnly;

@SideOnly(Side.CLIENT)
public class ConfirmRecordingDialog extends GuiScreenConfirmation {
	private ClientTickHandler tickHandler;

	public ConfirmRecordingDialog(ClientTickHandler tickHandler)
	{
		super(null, GuiScreenConfirmationType.Warning, "About to " + (tickHandler.isRecording() ? "stop" : "start") + " recording.", "Please confirm.", 1);
		this.tickHandler = tickHandler;
	}

	protected void actionPerformed(GuiButton buttonClicked)
	{
		this.mc.displayGuiScreen(null);
		
		if (buttonClicked.id == 0) {
			tickHandler.toggleRecording();
		}
	}
}
