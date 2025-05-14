# terminal mode Blues

yeah, terminal mode. it's a wacky world in there.
we seem to be unable to access input from the keyboard when we go into `terminal mode` in the VDP.
Neither of the two methods in `src/term.c` work.

_but hold the phone, looking thru the FabGL source late today (13 May 2025), there is a note about an escape hatch Ctrl-Alt-F12 (!) and I'll be damned but it worked - that code above when it launches and "hangs", if you Ctrl-Alt-(FN)-F12 (I'm on a mac, so I have to FN along with function key12 to get it do the function key instead of Louder) it started working, inchar delivered each key stroke ascii-wise. And the when I Ctrl-Q, it quit, but back into a weird term mode, and then when I Ctrl-Alt-(FN)-F12 again, I got back to MOS. and things were fine. (!)  so if you sent via VDU an escape seq of whatever Ctrl-Alt-F12 is, would it drop you into the right mode and then again to get out of it?_

`github.com/AgonPlatform/agon-vdp/video/video.ino` line 274

```C++
case TerminalState::Enabling: {
			// Turn on the terminal
			Terminal = std::unique_ptr<fabgl::Terminal>(new fabgl::Terminal());
			Terminal->begin(_VGAController.get());
			Terminal->connectSerialPort(VDPSerial);
			Terminal->enableCursor(true);
			// onVirtualKey is triggered whenever a key is pressed or released
			Terminal->onVirtualKeyItem = [&](VirtualKeyItem * vkItem) {
				if (vkItem->vk == VirtualKey::VK_F12) {
					if (vkItem->CTRL && (vkItem->LALT || vkItem->RALT)) {
						// CTRL + ALT + F12: emergency exit terminal mode
						stopTerminal();
					}
				}
			};
```

It seems what happens is the termmode is cancelled by the `stopTerminal()` and then re-started by the second `vdp_terminal_mode();`

which is why the Ctrl-Alt-F12 is sent again. This is no help in decyphering _why_ term mode's input is unavailable.
