"""Main script that GameDirector gets to run at the initialization"""

from syaget import *
#import math

#PrintHelp('all')


print('Hello from Python Script specialized for GameDirector, running at root space.')
#print('Doc String--------------: *{}*'.format(__doc__))


def Initialize(id):
    """Initialize this object, set number 
    of tries and activate timer"""

    print('Initializing Ponger GameDirector.')

    GameDirector.numTries = 5

    timer.Activate(id, SecondsToMicro(2.0), FireTimer.Repeat)


def Update(id, gameTime: GameClock, scriptComponent: PythonComponent):

    #print('Doc String------------: *{}*'.format(GameClock.__doc__))
    sid = idspace.BurnId()
    #sc = EntityCoord.AddScriptComponent(sid)
    #print('hello')
    
    GameDirector.numTries -= 1

    print('Executed Update(...) at time: {} with {} attempts left.'.format(MicroToMilli(gameTime.LogicTime), GameDirector.numTries))

    #if GameDirector.numTries == 0:
    #    timer.Stop(id)

# $(DataFolder)/Assets/Scripts/coordinator.py
#print("===== End of Script: '{}'.".format(__dir__)
