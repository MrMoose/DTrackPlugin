# DTrackPlugin

## Table of Contents
1. [About](#about)
2. [Installation](#installation)
3. [Usage](#usage)

## About
This is a plugin for the Unreal Engine 4.14 with the purpose of native integration 
of the Advanded Realtime Tracking DTrack and DTrack2 tracking solutions.
It supports both Blueprint and native C++ usage.

## Installation
To use this, setup your Unreal Project as a C++ Project (This doesn't mean there's 
need to code. The project can still focus on Blueprint usage). In your project's root folder,
create a directory named `Plugins` if it is not already there. Then clone or checkout 
this repository into this folder and regenerate your project. That's all.

## Usage
Using this functionality is generally focused on enhancing any Actor with tracking information. 
This can happen in either C++ or Blueprint. There's usually 3 steps involved:

1. Add a `DTrackComponent` to your actor. This component will act as glue between your actor and the underlying plug-in. It also configures the plugin with the settings for your DTrack system such as server hostname or port. Please note that at this point you can only connect to one such endpoint. You can have multiple components but they should point to the same server. Any of them will be chosen to setup tracking.
2. Add the `IDTrackInterface` to your actor. Implementing this interface will allow you to react to the different kinds of tracking data coming in.
3. Implement the desired events on the interface as you choose

### Native C++
In order to make your project depend on the plugin you may have to add the following to your module's `build.cs` script:

```csharp
public MyProject(TargetInfo Target) {

   PublicDependencyModuleNames.AddRange(new string[] {
      "Core",
      "CoreUObject",
      ...
      "DTrackPlugin"
   });

   PrivateIncludePaths.AddRange(new string[] {
      ...
      "DTrackPlugin/Public"
   });
}
```

In C++ based actors you should add `DTrackComponent` to your actor as a `UPROPERTY`. Access specifiers must include write access by the Editor. This is needed so you can modify the component's server settings so don't omit that unless you plan on setting those in code as well. Here's how this could look like: 

```c++
#include "DTrackComponent.h"
#include "DTrackInterface.h"

UCLASS()
class MYPROJECT_API AMyDTrackUsingActor : public AActor, public IDTrackInterface {

   GENERATED_BODY()
 
   public:
      ...
      UPROPERTY(VisibleAnywhere, Category = DTrack)  // Choose your own property specifiers according to your needs.
      UDTrackComponent *m_dtrack_component;
      
      /// Those are the actual event implementations
      
		/// This implements the IDTrackInterface event for body tracking data coming in
		void OnBodyData_Implementation(const int32 BodyID, const FVector &Position, const FRotator &Rotation) override;

		/// This implements the IDTrackInterface event for flystick tracking data coming in
		void OnFlystickData_Implementation(const int32 FlystickID, const FVector &Position, const FRotator &Rotation) override;

		/// This implements the IDTrackInterface event for flystick tracking data coming in
		void OnFlystickButton_Implementation(const int32 FlystickID, const int32 &ButtonIndex, const bool Pressed) override;
		void OnFlystickJoystick_Implementation(const int32 FlystickID, const TArray<float> &JoystickValues);

      /// There may be more as the plug-in gains functionality. See the interface docs for details. 
}
```

Note the `_Implementation` suffices. They are required to be able to override those interface's abstract methods. Don't omit them.

Now in your implementation file you must create the component and implement the interface functions as required. Here's how this could look like:


```c++
AMyDTrackUsingActor::AMyDTrackUsingActor() {
   ...
	m_dtrack_component = CreateDefaultSubobject<UDTrackComponent>(TEXT("DTrackComponent"));
	if (!m_dtrack_component) {
		UE_LOG(DTrack, Warning, TEXT("DTrack Component could not be created"));  // This shouldn't happen but you may treat it.
	}
}


void AMyDTrackUsingActor::OnBodyData_Implementation(const int32 BodyID, const FVector &Position, const FRotator &Rotation) {

	if (BodyID == 0) {        // The ID of the tracked body. There can be many, depending on your setup.
		FVector tmp = Position;
		tmp.X = tmp.X - 200;
		tmp.Z = tmp.Z + 200;
                             // well, do whatever you like with the data. Much flexibility here really.
		SetActorLocation(tmp, false, nullptr, ETeleportType::None);
		SetActorRotation(Rotation.Quaternion(), ETeleportType::None);
	}
}

void AMyDTrackUsingActor::OnFlystickJoystick_Implementation(const int32 FlystickID, const TArray<float> &JoystickValues) {

	FString out;  // just output those values for info.
	for (int32 i = 0; i < JoystickValues.Num(); i++) {
		out.Append(FString::Printf(TEXT(" %i: %f"), i, JoystickValues[i]));
	}

	UE_LOG(DTrack, Display, TEXT("%i joysticks - %s "), JoystickValues.Num(), *out);
}

```


## License
Copyright (c) 2017, Advanced Realtime Tracking GmbH
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
