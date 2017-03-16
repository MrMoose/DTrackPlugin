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

#include <memory>
#include <string>
#include <vector>

class FDTrackPollThread;

class FDTrackPollThread : public FRunnable {

	public:
		FDTrackPollThread(const UDTrackComponent *n_client, FDTrackPlugin *n_plugin);
		~FDTrackPollThread();
		
		/** Singleton instance, can access the thread any time via static accessor
		 */
		static  FDTrackPollThread *m_runnable;

	

		/*
		Start the thread and the worker from static (easy access)!
		This code ensures only 1 Prime Number thread will be able to run at a time.
		This function returns a handle to the newly started instance.
		*/
		static FDTrackPollThread* start(const UDTrackComponent *n_client, FDTrackPlugin *n_plugin);

	
		void interrupt();
		void join();


		bool Init() override;

		// 1 is success
		// 0 is failure
		uint32 Run() override;

		/// This is called if a thread is requested to terminate early.
		void Stop() override;

		void Exit() override;


	private:

		/// after receive, treat body info and send it to the plug-in
		void handle_bodies();

		/// translate dtrack rotation matrix to rotator according to selected room calibration
		FRotator from_dtrack_rotation(const double(&n_matrix)[9]);

		/// translate dtrack translation to unreal space
		FVector from_dtrack_location(const double(&n_translation)[3]);

		
		FRunnableThread   *m_thread; //!< Thread to run the worker FRunnable on

		FDTrackPlugin     *m_plugin; //!< during runtime, plugin gets data injected

		/// this is the DTrack SDK main object. I'll have one one owned here as Í do not know if they can coexist
		std::unique_ptr< DTrackSDK > m_dtrack;

		/// parameters
		const bool                   m_dtrack2;
		const std::string            m_dtrack_server_ip;
		const uint32                 m_dtrack_server_port;
		const ECoordinateSystemType  m_coordinate_system = ECoordinateSystemType::CST_Normal;


		/** Stop this thread? Uses Thread Safe Counter */
		FThreadSafeCounter           m_stop_counter;



		/** 
		 * each flystick gets its button states remembered here.
		 * flystick's ID is index in vector
		 */
		std::vector< std::vector<int> > m_flystick_buttons;


		/// room coordinate adoption matrix for "normal" setting
		const FMatrix  m_trafo_normal;

		/// transposed variant cached
		const FMatrix  m_trafo_normal_transposed;

		/// room coordinate adoption matrix for "power wall" setting
		const FMatrix  m_trafo_powerwall;

		/// transposed variant cached
		const FMatrix  m_trafo_powerwall_transposed;

		/// room coordinate adoption matrix for "unreal adapted" setting
		const FMatrix  m_trafo_unreal_adapted;

		/// transposed variant cached
		const FMatrix  m_trafo_unreal_adapted_transposed;

};