yaget.AddStages({ 'Main Menu', 'InGame Menu', 'Level 1', 'Level 2' })

-- testing all available components
local itemId = yaget.NewItemId()

yaget.InputComponent(itemId, {'Input1', 'Input2', 'Input3'})
yaget.MenuComponent(itemId, 'Main Screen')
yaget.UnitComponent(itemId, 42)
yaget.ScriptComponent(itemId, 'Main Menu', 'Scripts@MainMenu')
yaget.NameComponent(itemId, 'Main Menu')
yaget.MaterialComponent(itemId, 'SceneItems@SpaceBackground')
yaget.BulletComponent(itemId)
yaget.TextComponent(itemId)
yaget.MenuScreenComponent(itemId, {'New Game', 'Load Game', 'Options', 'Credits'})
yaget.VelocityComponent(itemId)
yaget.LocationComponent3(itemId, yaget.Vector3.new(42, 69, -1), yaget.Quaternion.Identity, yaget.Vector3.One)
yaget.StageComponent(itemId, 'Boot', yaget.StageBlendOp.Replace)

yaget.AddStageItems('Main Menu', { itemId })


--[[
New Item
Name: Background
LocationComponent3: {1.00, 2.00, 3.00};{4.00, 5.00, 6.00, 7.00};{8.00, 9.00, 10.00}
StageName: Boot, Main Menu, Level 1, Level 2
MaterialComponent: SceneItems@SpaceBackground

New Item
Name: Player
LocationComponent3: {0.5, 0.1, 0.00};{0.00, 0.00, 0.00, 1.00};{1.00, 1.00, 1.00}
InputComponent: Move Left, Move Right, Shoot
StageName: Level 1, Level 2
VelocityComponent: 
MaterialComponent: SceneItems@Defensor-Ship

New Global Item
StageComponent: Boot; Replace

New Global Item
MenuComponent: MainMenu
StageName: Main Menu, InGame Menu

New Item
Name: Menu Input
InputComponent: Next Menu Item, Previous Menu Item, Select Menu Item
ScriptComponent Main Menu; Menues@MainMenu
StageName: Main Menu, InGame Menu

Stages
Boot
Main Menu
InGame Menu
Level 1
Level 2
]]