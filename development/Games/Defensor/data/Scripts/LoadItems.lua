yaget.AddStages({ 'Main Menu', 'InGame Menu', 'Level 1', 'Level 2' })

-- testing all available components
itemId = yaget.NewItemId()

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
