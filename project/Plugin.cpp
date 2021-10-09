#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "Behaviours.h"

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "BotNameTEST";
	info.Student_FirstName = "Jonas";
	info.Student_LastName = "De Ridder";
	info.Student_Class = "2DAE07";

	//Called when the plugin is loaded
	m_pSeek = new Seek();
	m_pWander = new Wander();
	m_pFlee = new Flee();
	m_pFace = new Face();

	//Setups blackboard
	m_pBlackboard = new Blackboard();
	//Steering
	m_pBlackboard->AddData("SteeringBehavior", &m_pSteeringBehavior);
	m_pBlackboard->AddData("Seek", m_pSeek);
	m_pBlackboard->AddData("Wander", m_pWander);
	m_pBlackboard->AddData("Flee", m_pFlee);
	m_pBlackboard->AddData("Face", m_pFace);
	m_pBlackboard->AddData("SteeringCooldown", 0.f);
	m_pBlackboard->AddData("SteeringCooldownRemaining", 0.f);
	m_pBlackboard->AddData("VariableSteeringCooldown", 0.f);
	TargetData target{};
	m_pBlackboard->AddData("Target", target);
	m_pBlackboard->AddData("IntermediateTarget", target);
	
	//Agent info
	AgentInfo agent{ m_pInterface->Agent_GetInfo() };
	m_pBlackboard->AddData("Agent", agent);
	m_pBlackboard->AddData("AgentHistory", &m_AgentHistory);
	m_pBlackboard->AddData("PreviousAgentHistoryIndex", &m_PreviousAgentHistoryIndex);
	m_pBlackboard->AddData("RunMode", false);

	m_AgentHistory.resize(m_AgentHistorySize);

	//Inventory
	m_pInventory = new Inventory(m_pInterface);
	m_pBlackboard->AddData("Inventory", m_pInventory);

	//World info
	std::vector<HouseInfo> houses{};
	Elite::Vector2 houseEnteredAt{};
	m_pBlackboard->AddData("Houses", houses);
	m_pBlackboard->AddData("HouseEnteredAt", houseEnteredAt);
	m_pBlackboard->AddData("TimeInHouse", 0.f);
	m_pBlackboard->AddData("EnteredHouses", &m_HousesEntered);
	m_pBlackboard->AddData("Path", &m_Path);
	m_pBlackboard->AddData("CurrentPathNode", &m_CurrentPathNode);

	TargetData locationToCheckOut{};
	m_pBlackboard->AddData("LocationToCheckOut", locationToCheckOut);

	std::vector<EntityInfo> entities{};
	m_pBlackboard->AddData("Entities", entities);

	m_pBlackboard->AddData("WorldInfo", m_pInterface->World_GetInfo());

	//Enemies
	m_pBlackboard->AddData("EnemiesLastSeen", &m_EnemiesLastSeen);
	m_pBlackboard->AddData("EnemyMemoryTime", m_EnemyMemoryTime);
	m_pBlackboard->AddData("RememberedEnemyFleeRange", m_RememberedEnemyFleeRange);
	m_pBlackboard->AddData("RememberFleeLocation", Elite::Vector2{});
	m_pBlackboard->AddData("RememberFleeLocationWeight", 0.f);

	//Add interface to blackboard
	m_pBlackboard->AddData("Interface", m_pInterface);

	m_Path.push_back({ -86,27 });
	m_Path.push_back({ -110,110 });
	m_Path.push_back({ 32,68 });
	m_Path.push_back({ 120,0 });
	m_Path.push_back({ 87,-103 });
	m_Path.push_back({ -80,-90 });

	m_pBehaviorTree = new BehaviorTree(m_pBlackboard,
		new BehaviorPersistentSequence({
			//Steering
			new BehaviorSelector({
				new BehaviorSequence({
					new BehaviorConditional(agentInPurgeZone),
					new BehaviorAction(ChangeToFlee),
					new BehaviorAction(StartRunning)
				}),
				new BehaviorSequence({
					new BehaviorConditional(isEnemyInFOV),
					new BehaviorSelector({
						new BehaviorSequence({
							new BehaviorConditional(agentHasPistol),
							new BehaviorAction(FaceEnemy)
						}),
						new BehaviorSequence({
							new BehaviorAction(ChangeToFlee),
							new BehaviorConditional(agentEneryOverHalf),
							new BehaviorAction(StartRunning)
						})
					})
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringIsFace, true),
					new BehaviorConditional(isItemInFOV),
					new BehaviorAction(ChangeToSeek)
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringIsFace, true),
					new BehaviorConditional(agentInHouse),
					new BehaviorConditional(isItemInFOV, true),
					new BehaviorConditional(agentBeenInHouseLongEnough),
					new BehaviorAction(ExitHouse)
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringIsFace, true),
					new BehaviorConditional(isHouseInFOV),
					new BehaviorAction(ChangeToSeek)
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringIsFace, true),
					new BehaviorConditional(agentIsReachingWorldBounds),
					new BehaviorAction(ChangeToSeek)
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringIsFace, true),
					new BehaviorConditional(remembersLocationToCheckOut),
					new BehaviorAction(ChangeToSeek)
				}),
				new BehaviorSequence({
					new BehaviorConditional(SteeringOnCooldown, true),
					new BehaviorAction(UpdateWorldPath)
				})
			}),
			// Items
			new BehaviorSequence({
				new BehaviorConditional(agentHasPistol),
				new BehaviorConditional(agentShouldShoot),
				new BehaviorAction(ShootPistol)
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentHasMedkit),
				new BehaviorAction(RestoreHealth)
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentHasFood),
				new BehaviorAction(RestoreEnergy)
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentInHouse, true),
				new BehaviorConditional(agentHasAnyFood),
				new BehaviorConditional(agentEneryOverHalf),
				new BehaviorConditional(agentStaminaFull),
				new BehaviorAction(StartRunning)
			}),
			new BehaviorSequence({
				new BehaviorConditional(isItemInRange),
				new BehaviorAction(GrabItem)
			}),
			//When enemy is Bitten this frame, remember that location and if possible start running
			new BehaviorSequence({
				new BehaviorConditional(agentBittenNow),
				new BehaviorConditional(SteeringIsFace, true),
				new BehaviorSelector({
					new BehaviorSequence({
						new BehaviorConditional(agentHasPistol),
						new BehaviorAction(TurnAround)
					}),
					new BehaviorSequence({
						new BehaviorAction(RunFromDamagingEnemy),
						new BehaviorConditional(agentCanRun),
						new BehaviorAction(StartRunning)
					})
				})
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentHasPistol, true),
				new BehaviorConditional(SteeringIsFace, true),
				new BehaviorConditional(isPurgeZoneInFOV, true),
				new BehaviorConditional(remembersEnemies),
				new BehaviorAction(UpdateTargetWithEnemyMemory)
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentIsRunning),
				new BehaviorConditional(agentCanRun, true),
				new BehaviorAction(StopRunning)
			}),
			new BehaviorSequence({
				new BehaviorConditional(agentEnteredHouseNow),
			}),
		})
	);

}

//Called only once
void Plugin::DllInit()
{
}

//Called only once
void Plugin::DllShutdown()
{
	//Called wheb the plugin gets unloaded
	SAFE_DELETE(m_pSeek);
	SAFE_DELETE(m_pWander);
	SAFE_DELETE(m_pFlee);
	SAFE_DELETE(m_pFace);
	SAFE_DELETE(m_pInventory);
	SAFE_DELETE(m_pBehaviorTree);
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
	params.AutoGrabClosestItem = false; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_M))
	{
		m_Target = {0,0};
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	auto agentInfo = m_pInterface->Agent_GetInfo();
	m_pBlackboard->ChangeData("Agent", agentInfo);

	//Update data
	float SteeringCooldown{};
	m_pBlackboard->GetData("SteeringCooldownRemaining", SteeringCooldown);
	m_pBlackboard->ChangeData("SteeringCooldownRemaining", SteeringCooldown - dt);

	float houseElapsed{};
	m_pBlackboard->GetData("TimeInHouse", houseElapsed);
	if (agentInfo.IsInHouse) m_pBlackboard->ChangeData("TimeInHouse", houseElapsed + dt);
	else m_pBlackboard->ChangeData("TimeInHouse", 0.f);

	std::for_each(m_HousesEntered.begin(), m_HousesEntered.end(), [dt](LastSeen& lastSeen) {lastSeen.timeElapsed += dt; });
	float houseMemoryTime{ m_HouseMemoryTime };
	m_HousesEntered.erase(std::remove_if(m_HousesEntered.begin(), m_HousesEntered.end(), [houseMemoryTime](const LastSeen& lastSeen) {
		return lastSeen.timeElapsed >= houseMemoryTime;
	}), m_HousesEntered.end());

	std::for_each(m_EnemiesLastSeen.begin(), m_EnemiesLastSeen.end(), [dt](LastSeen& lastSeen) {
		lastSeen.timeElapsed += dt; 
		lastSeen.PredictedLocation = lastSeen.SeenLocation + lastSeen.timeElapsed * lastSeen.Velocty;
	});
	float enemyMemoryTime{ m_EnemyMemoryTime };
	float enemyWorryRange{ m_RememberedEnemyFleeRange };
	m_EnemiesLastSeen.erase(
		std::remove_if(m_EnemiesLastSeen.begin(), m_EnemiesLastSeen.end(),
			[enemyMemoryTime, enemyWorryRange, &agentInfo](const LastSeen& lastSeen) {
				return lastSeen.timeElapsed >= enemyMemoryTime || 
					Elite::DistanceSquared(lastSeen.PredictedLocation, agentInfo.Position) >= enemyWorryRange * enemyWorryRange; 
			}
		),
		m_EnemiesLastSeen.end()
	);

	// Steering
	auto steering = SteeringPlugin_Output();


	auto nextTargetPos = m_Target; //To start you can use the mouse position as guidance

	auto vHousesInFOV = GetHousesInFOV();
	auto vEntitiesInFOV = GetEntitiesInFOV();

	m_pBlackboard->ChangeData("Houses", vHousesInFOV);
	m_pBlackboard->ChangeData("Entities", vEntitiesInFOV);

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			std::cout << "Purge Zone in FOV:" << e.Location.x << ", "<< e.Location.y <<  " ---EntityHash: " << e.EntityHash << "---Radius: "<< zoneInfo.Radius << std::endl;
		}
	}

	m_pBehaviorTree->Update(dt);

	TargetData target{};
	m_pBlackboard->GetData("Target", target);

	//if (!Elite::AreEqual(m_Target.x, 0.f) || !Elite::AreEqual(m_Target.y, 0.f)) target.Position = m_Target;

	if (m_pSteeringBehavior == m_pFlee)
	{
		//If fleeing, agent should take in surroundings => Seek to localy inverted target
		Elite::Vector2 agentToTarget{ target.Position - agentInfo.Position };
		target.Position += -2 * agentToTarget;
		m_pSteeringBehavior = m_pSeek;
	}

	if(m_pSteeringBehavior != m_pFace) target.Position = m_pInterface->NavMesh_GetClosestPathPoint(target.Position);
	m_pSteeringBehavior->SetTarget(target);

	m_Target = target.Position;
	steering = m_pSteeringBehavior->CalculateSteering(dt, agentInfo);
	//steering = m_pFace->CalculateSteering(dt, agentInfo);
	m_pBlackboard->GetData("RunMode", steering.RunMode);

	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	m_PreviousAgentHistoryIndex = (m_PreviousAgentHistoryIndex + 1) % m_AgentHistorySize;
	m_AgentHistory[m_PreviousAgentHistoryIndex] = agentInfo;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	TargetData target{};
	m_pBlackboard->GetData("Target", target);
	m_pInterface->Draw_SolidCircle(target.Position, 1.f, { 0,0 }, { 1, 0, 0 });
	Elite::Vector2 rememberedFleeLocation{};
	m_pBlackboard->GetData("RememberFleeLocation", rememberedFleeLocation);
	m_pInterface->Draw_SolidCircle(rememberedFleeLocation, 0.7f, {}, { 0,0,1 });
	m_pInterface->Draw_Circle(m_Target, 1.7f, { 0,1,0 });

	auto worldInfo = m_pInterface->World_GetInfo();
	Elite::Vector2 worldPoints[4] = {
		{worldInfo.Center + Elite::Vector2{worldInfo.Dimensions.x, worldInfo.Dimensions.y}},
		{worldInfo.Center + Elite::Vector2{-worldInfo.Dimensions.x, worldInfo.Dimensions.y}},
		{worldInfo.Center + Elite::Vector2{-worldInfo.Dimensions.x, -worldInfo.Dimensions.y}},
		{worldInfo.Center + Elite::Vector2{worldInfo.Dimensions.x, -worldInfo.Dimensions.y}},
	};
	m_pInterface->Draw_Polygon(worldPoints, 4, { 1,0,0 });

	const float tooCloseToBorderRange{ 125.f }; //Corresponds to Behaviors.h agentIsReachingWorldBounds
	Elite::Vector2 agentBounds[4] = {
		{worldInfo.Center + Elite::Vector2{worldInfo.Dimensions.x - tooCloseToBorderRange, worldInfo.Dimensions.y - tooCloseToBorderRange}},
		{worldInfo.Center + Elite::Vector2{-worldInfo.Dimensions.x + tooCloseToBorderRange, worldInfo.Dimensions.y - tooCloseToBorderRange}},
		{worldInfo.Center + Elite::Vector2{-worldInfo.Dimensions.x + tooCloseToBorderRange, -worldInfo.Dimensions.y + tooCloseToBorderRange}},
		{worldInfo.Center + Elite::Vector2{worldInfo.Dimensions.x - tooCloseToBorderRange, -worldInfo.Dimensions.y + tooCloseToBorderRange}},
	};
	m_pInterface->Draw_Polygon(agentBounds, 4, { 1,1,1 });

}

vector<HouseInfo> Plugin::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;
}

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}
