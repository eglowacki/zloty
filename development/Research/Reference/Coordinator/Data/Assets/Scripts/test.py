"""testing python script running
on component and getting ticked
every frame"""

from syaget import *
import math

print('Hello from Python.')

def Update(id, gameTime: GameClock, scriptComponent: PythonComponent, locationComponent: LocationComponent):

    elapsedSeconds = MicroToSeconds(gameTime.LogicTime - locationComponent.BeginLife()) * 4.0
    interpolatedScale = ((math.sin(elapsedSeconds) + 1.0) * 0.5) + 0.5

    locationComponent.SetScale(Vector3(interpolatedScale, interpolatedScale, interpolatedScale))

    interpolatedRotation = math.modf(elapsedSeconds / 64.0)[0] * 360.0

    quat = Quaternion.CreateFromAxisAngle(Vector3(0, 0, 1), DegToRad(interpolatedRotation))
    locationComponent.SetOrientation(quat)

    newPos = Vector3.Transform(Vector3(0.5, 0.0, 0.0), quat)
    locationComponent.SetPosition(newPos);
