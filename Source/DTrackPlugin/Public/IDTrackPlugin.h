// Copyright (c) 2017, Advanced Realtime Tracking GmbH
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(DTrackPluginLog, Log, All);

/**
 * The public interface to this module
 */
class IDTrackPlugin : public IModuleInterface {

	public:

		/**
		* Singleton-like access to this module's interface.  This is just for convenience!
		* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
		*
		* @return Returns singleton instance, loading the module on demand if needed
		*/
		static inline IDTrackPlugin& Get() {
		
			return FModuleManager::LoadModuleChecked< IDTrackPlugin >("DTrackPlugin");
		}

		/**
		* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
		*
		* @return True if the module is loaded and ready to use
		*/
		static inline bool IsAvailable() {
			
			return FModuleManager::Get().IsModuleLoaded( "DTrackPlugin" );
		}

	
		/** 
		 * Initialize internal structures with component as client
		 * Will setup and connect internal SDK and connect it to the server and port
		 * specified in client component.
		 *
		 */
		virtual void start_up(class UDTrackComponent *n_client) = 0;

		/**
		 * Counterpart to start_up. Tell the plug-in this component is no longer 
		 * interested in tracking data. If this is the last component, tracking will stop
		 */
		virtual void remove(class UDTrackComponent *n_client) = 0;

		/// begin measurement by starting to receive tracking data
		virtual void begin_tracking() = 0;

		/// client components call this to HUP the plug-in and cause it to tick,
		/// which it doesn't seem to be able to do on its own.
		virtual void tick(const float n_delta_time, const UDTrackComponent *n_component) = 0;

		virtual bool IsRemoteEnabled() = 0;
};
