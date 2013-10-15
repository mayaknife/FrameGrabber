package gooroos.framegrabber;

import cpw.mods.fml.common.Mod;
import cpw.mods.fml.common.Mod.EventHandler;
import cpw.mods.fml.common.Mod.Instance;
import cpw.mods.fml.common.SidedProxy;
import cpw.mods.fml.common.event.FMLInitializationEvent;
import cpw.mods.fml.common.event.FMLPostInitializationEvent;
import cpw.mods.fml.common.event.FMLPreInitializationEvent;
import cpw.mods.fml.common.network.NetworkMod;
import cpw.mods.fml.common.registry.TickRegistry;
import cpw.mods.fml.relauncher.Side;

import gooroos.framegrabber.client.ClientTickHandler;

@Mod(modid="FrameGrabber", name="FrameGrabber", version="0.1.0")
@NetworkMod(clientSideRequired=true)
public class FrameGrabber {

        // The instance of your mod that Forge uses.
        @Instance("FrameGrabber")
        public static FrameGrabber instance;
       
        // Says where the client and server 'proxy' code is loaded.
        @SidedProxy(clientSide="gooroos.framegrabber.client.ClientProxy", serverSide="gooroos.frameGrabber.CommonProxy")
        public static CommonProxy proxy;
       
        @EventHandler
        public void preInit(FMLPreInitializationEvent event) {
        	ClientTickHandler handler = new ClientTickHandler();
        	
        	TickRegistry.registerTickHandler(handler, Side.CLIENT);    	
        	proxy.registerKeyBindings(handler);
        }
       
        @EventHandler
        public void load(FMLInitializationEvent event) {
            // Don't really need this yet.
        }
       
        @EventHandler
        public void postInit(FMLPostInitializationEvent event) {
            // Don't really need this yet.
        }
}