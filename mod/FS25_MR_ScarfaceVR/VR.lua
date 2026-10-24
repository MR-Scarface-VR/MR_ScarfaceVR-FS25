MR_ScarfaceVR = {}

function MR_ScarfaceVR:loadMap()
    print("MR_ScarfaceVR: experimental native stereo adapter by MR_Scarface; FS25 1.25.0.0 / Meta Quest Link")
end

function MR_ScarfaceVR:update(dt)
    if type(ownVRMenu) ~= "function" or type(ownVRWorld) ~= "function" then
        if not self.warned then
            print("MR_ScarfaceVR: native adapter unavailable; desktop mode retained")
            self.warned = true
        end
        return
    end
    local world = false
    local camera = nil
    local player = g_localPlayer
    local menu = g_gui ~= nil and g_gui:getIsGuiVisible()
    if g_currentMission ~= nil and player ~= nil and not menu then
        local vehicle = player:getCurrentVehicle()
        if vehicle ~= nil and vehicle.getActiveCamera ~= nil then
            camera = vehicle:getActiveCamera()
            world = camera ~= nil and camera.isInside == true
        elseif player.isControlled and player.camera ~= nil then
            camera = player.camera
            world = camera.isFirstPerson == true and not camera.isInConversation
        end
    end
    local cameraNode = getCamera()
    world = world and cameraNode ~= nil and cameraNode ~= 0 and not getIsOrthographic(cameraNode)
    -- Mode is refreshed each update so unloading a map cannot leave a stale world view.
    if world then ownVRWorld() else ownVRMenu() end
end

function MR_ScarfaceVR:deleteMap()
    if type(ownVRMenu) == "function" then ownVRMenu() end
end

addModEventListener(MR_ScarfaceVR)
