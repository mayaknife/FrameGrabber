package gooroos.framegrabber.client;

import org.lwjgl.input.Keyboard;

import net.minecraft.client.settings.KeyBinding;
import cpw.mods.fml.client.registry.KeyBindingRegistry;
import cpw.mods.fml.common.ITickHandler;

import gooroos.framegrabber.CommonProxy;


public class ClientProxy extends CommonProxy {
	public void registerKeyBindings(ITickHandler handler)
	{
		KeyBinding[] bindings = { new KeyBinding("Toggle Recording", Keyboard.KEY_R) };
		boolean[] repeats = { false };
		
		KeyBindingRegistry.registerKeyBinding(new GrabberKeyHandler(bindings, repeats, (ClientTickHandler)handler));
	}
}
