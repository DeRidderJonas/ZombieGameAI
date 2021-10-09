#pragma once
/*=============================================================================*/
// Copyright 2020-2021 Elite Engine
/*=============================================================================*/
// Behaviors.h: Implementation of certain reusable behaviors for the BT version of the Agario Game
/*=============================================================================*/
#ifndef ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
#define ELITE_APPLICATION_BEHAVIOR_TREE_BEHAVIORS
//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include "stdafx.h"
#include "EBehaviorTree.h"
#include "SteeringBehaviors.h"
//-----------------------------------------------------------------
// Helper functions
//-----------------------------------------------------------------

void GetEntitiesOfTypeInFOV(std::vector<EntityInfo>& entities, eEntityType type, IExamInterface* pInterface)
{
	entities.erase(std::remove_if(entities.begin(), entities.end(), [&type](const EntityInfo& entity) {return entity.Type != type; }), entities.end());
}

void GetEnemiesInFOV(std::vector<EntityInfo>& entities, std::vector<EnemyInfo>& enemies, IExamInterface* pInterface)
{
	GetEntitiesOfTypeInFOV(entities, eEntityType::ENEMY, pInterface);
	enemies.resize(entities.size());
	std::transform(entities.begin(), entities.end(), enemies.begin(), [&pInterface](const EntityInfo& entity) {
		EnemyInfo enemy{};
		pInterface->Enemy_GetInfo(entity, enemy);
		return enemy;
	});
}

void GetItemsInFOV(std::vector<EntityInfo>& entities, std::vector<ItemInfo>& items, IExamInterface* pInterface)
{
	GetEntitiesOfTypeInFOV(entities, eEntityType::ITEM, pInterface);
	items.resize(entities.size());
	std::transform(entities.begin(), entities.end(), items.begin(), [&pInterface](const EntityInfo& entity) {
		ItemInfo item{};
		pInterface->Item_GetInfo(entity, item);
		return item;
	});
}

void GetPurgeZonesInFOV(std::vector<EntityInfo>& entities, std::vector<PurgeZoneInfo>& purgeZones, IExamInterface* pInterface)
{
	GetEntitiesOfTypeInFOV(entities, eEntityType::PURGEZONE, pInterface);
	purgeZones.resize(entities.size());
	std::transform(entities.begin(), entities.end(), purgeZones.begin(), [&pInterface](const EntityInfo& entity) {
		PurgeZoneInfo purgeZone{};
		pInterface->PurgeZone_GetInfo(entity, purgeZone);
		return purgeZone;
	});
}

bool AgentIsHoldingItem(Elite::Blackboard* pBlackboard, eItemType itemType, bool ignoreAgentState = false)
{
	Inventory* pInventory = nullptr;
	pBlackboard->GetData("Inventory", pInventory);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	int itemSlot{ -1 };
	switch (itemType)
	{
	case eItemType::PISTOL:
		pInventory->GetPistol(itemSlot);
		break;
	case eItemType::MEDKIT:
		pInventory->GetHealthpack(itemSlot, agent.Health, ignoreAgentState);
		break;
	case eItemType::FOOD:
		pInventory->GetFood(itemSlot, agent.Energy, ignoreAgentState);
		break;
	}

	return itemSlot >= 0;
}

//Returns index to duplicate
int AgentIsHoldingDuplicate(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);

	UINT capacity{ pInterface->Inventory_GetCapacity() };
	int amountOfFood{}, amountOfMedkits{}, amountOfPistols{};
	for (UINT i = 0; i < capacity; i++)
	{
		ItemInfo item{};
		if (!pInterface->Inventory_GetItem(i, item)) continue;

		switch (item.Type)
		{
		case eItemType::FOOD:
			amountOfFood++;
			if (amountOfFood > 1) return i;
			break;
		case eItemType::MEDKIT:
			amountOfMedkits++;
			if (amountOfMedkits > 1) return i;
			break;
		case eItemType::PISTOL:
			amountOfPistols++;
			if (amountOfPistols > 1) return i;
			break;
		}
	}

	return -1;
}

void AddHouseToEnteredHouses(Elite::Blackboard* pBlackboard, const HouseInfo& house)
{
	const float sameEnemyLocationMargin{ 1.f };
	std::vector<LastSeen>* pHousesEntered = nullptr;
	pBlackboard->GetData("EnteredHouses", pHousesEntered);

	if (pHousesEntered->size() == 0)
	{
		pHousesEntered->push_back({ house.Center });
		return;
	}

	auto foundIt = std::find(pHousesEntered->begin(), pHousesEntered->end(), house);

	//No last seen was close enough for it to be considered the same house
	if (foundIt == pHousesEntered->end())
	{
		pHousesEntered->push_back({ house.Center });
		return;
	}

}

void AddEnemySeenLocation(Elite::Blackboard* pBlackboard, const EnemyInfo& enemyInfo)
{
	const float sameEnemyLocationMargin{ 1.f };
	std::vector<LastSeen>* pLastSeenEnemies = nullptr;
	pBlackboard->GetData("EnemiesLastSeen", pLastSeenEnemies);

	if (pLastSeenEnemies->size() == 0)
	{
		pLastSeenEnemies->push_back({ enemyInfo.Location, enemyInfo.LinearVelocity });
		return;
	}

	auto foundIt = std::find(pLastSeenEnemies->begin(), pLastSeenEnemies->end(), enemyInfo);

	//No last seen location was close enough for it to be considered the same enemy
	if (foundIt == pLastSeenEnemies->end())
	{
		pLastSeenEnemies->push_back({ enemyInfo.Location, enemyInfo.LinearVelocity });
		return;
	}

	//Update location because it's probably the same enemy
	foundIt->SeenLocation = enemyInfo.Location;
	foundIt->Velocty = enemyInfo.LinearVelocity;
	foundIt->timeElapsed = 0.f;
}

//-----------------------------------------------------------------
//Agent Conditionals
//-----------------------------------------------------------------

bool agentWasDamaged(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	std::vector<AgentInfo>* agentHistory = nullptr;
	size_t* previousAgentHistoryIndex = nullptr;
	pBlackboard->GetData("Agent", agent);
	pBlackboard->GetData("AgentHistory", agentHistory);
	pBlackboard->GetData("PreviousAgentHistoryIndex", previousAgentHistoryIndex);

	return agent.Health < agentHistory->at(*previousAgentHistoryIndex).Health;
}

bool agentBittenNow(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	return agent.Bitten;
}

bool agentCanRun(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	return agent.Stamina > 0.25f;
}

bool agentIsRunning(Elite::Blackboard* pBlackboard)
{
	bool RunMode{};
	pBlackboard->GetData("RunMode", RunMode);

	return RunMode;
}

bool agentHasEnergy(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	return agent.Energy > 0.f;
}

bool agentEneryOverHalf(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	return agent.Energy > 5.f;
}

bool agentHasAnyFood(Elite::Blackboard* pBlackboard)
{
	return AgentIsHoldingItem(pBlackboard, eItemType::FOOD, true);
}

bool agentHasFood(Elite::Blackboard* pBlackboard)
{
	return AgentIsHoldingItem(pBlackboard, eItemType::FOOD);
}

bool agentStaminaFull(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	return agent.Stamina > 9.5f;
}

bool agentHasMedkit(Elite::Blackboard* pBlackboard)
{
	return AgentIsHoldingItem(pBlackboard, eItemType::MEDKIT);
}

bool agentHasPistol(Elite::Blackboard* pBlackboard)
{
	return AgentIsHoldingItem(pBlackboard, eItemType::PISTOL);
}

bool agentInHouse(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	std::vector<HouseInfo> houses{};
	pBlackboard->GetData("Houses", houses);

	//If it can see the house it's in, add it to entered house. House isnt always added the moment agent enters it.
	if(agent.IsInHouse && houses.size() > 0) AddHouseToEnteredHouses(pBlackboard, houses[0]);


	return agent.IsInHouse;
}

bool agentInPurgeZone(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);


	//Remove non enemies, transform entities to actual enemies
	std::vector<PurgeZoneInfo> purgeZones{};
	GetPurgeZonesInFOV(entities, purgeZones, pInterface);

	if (purgeZones.size() == 0) return false;

	bool agentInPurgeZone{ false };
	TargetData target{};
	for (const PurgeZoneInfo& purgeZone : purgeZones)
	{
		if (Elite::Distance(agent.Position, purgeZone.Center) < purgeZone.Radius)
		{
			agentInPurgeZone = true;
			target.Position = purgeZone.Center;

			pBlackboard->ChangeData("Target", target);
			pBlackboard->ChangeData("IntermediateTarget", target);
			pBlackboard->ChangeData("SteeringCooldownRemaining", 0.f);
		}
	}

	return agentInPurgeZone;
}

bool agentEnteredHouseNow(Elite::Blackboard* pBlackboard)
{
	std::vector<LastSeen>* housesEntered{};
	pBlackboard->GetData("EnteredHouses", housesEntered);
	std::vector<HouseInfo> houses{};
	pBlackboard->GetData("Houses", houses);

	AgentInfo agentNow{};
	std::vector<AgentInfo>* agentHistory = nullptr;
	size_t* previousAgentHistoryIndex = nullptr;
	pBlackboard->GetData("Agent", agentNow);
	pBlackboard->GetData("AgentHistory", agentHistory);
	pBlackboard->GetData("PreviousAgentHistoryIndex", previousAgentHistoryIndex);

	AgentInfo agentLastFrame{agentHistory->at(*previousAgentHistoryIndex)};

	if (agentNow.IsInHouse && !agentLastFrame.IsInHouse)
	{
		AgentInfo agent{};
		pBlackboard->GetData("Agent", agent);
		pBlackboard->ChangeData("HouseEnteredAt", agent.Position);
		pBlackboard->ChangeData("LocationToCheckOut", TargetData{});
		std::cout << "House entered" << '\n';
		if (houses.size() > 0) AddHouseToEnteredHouses(pBlackboard, houses[0]);
		return true;
	}

	return false;
}

bool agentIsReachingWorldBounds(Elite::Blackboard* pBlackboard)
{
	WorldInfo worldInfo{};
	pBlackboard->GetData("WorldInfo", worldInfo);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	const float tooCloseRange{ 125.f };
	const float tooCloseSqrd{ tooCloseRange * tooCloseRange };
	if (
		agent.Position.y > (worldInfo.Center.y + worldInfo.Dimensions.y - tooCloseRange) ||
		agent.Position.y < (worldInfo.Center.y - worldInfo.Dimensions.y + tooCloseRange) ||
		agent.Position.x > (worldInfo.Center.x + worldInfo.Dimensions.x - tooCloseRange) ||
		agent.Position.x < (worldInfo.Center.x - worldInfo.Dimensions.x + tooCloseRange))
	{
		std::cout << "Agent is reaching world bounds" << '\n';
		TargetData target{};
		pBlackboard->ChangeData("Target", target);
		pBlackboard->ChangeData("IntermediateTarget", target);
		const float seekWorldCenterTime{ 10.f };
		pBlackboard->ChangeData("VariableSteeringCooldown", seekWorldCenterTime);
		return true;
	}

	return false;
}

bool agentShouldShoot(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);

	//Remove non enemies, transform entities to actual enemies
	std::vector<EnemyInfo> enemies{};
	GetEnemiesInFOV(entities, enemies, pInterface);

	const float closeEnoughAngle{ 0.06f };

	for (const EnemyInfo& enemy : enemies)
	{
		Elite::Vector2 agentToEnemy{ enemy.Location - agent.Position };

		float distanceToEnemy{ agentToEnemy.Magnitude() };
		float distanceFOVPercentage{ 1 - distanceToEnemy / agent.FOV_Range };

		float angleToEnemy{ Elite::GetOrientationFromVelocity(agentToEnemy) };
		float agentOrientation{ Elite::GetOrientationFromVelocity(Elite::OrientationToVector(agent.Orientation)) };
		if (Elite::AreEqual(angleToEnemy, agentOrientation, closeEnoughAngle * (1 + distanceFOVPercentage))) return true;
	}

	return false;
}

bool agentBeenInHouseLongEnough(Elite::Blackboard* pBlackboard)
{
	float timeInHouse{};
	pBlackboard->GetData("TimeInHouse", timeInHouse);

	const float longEnough{ 5.f };
	return timeInHouse >= longEnough;
}
//-----------------------------------------------------------------
// Steering Conditionals
//-----------------------------------------------------------------
bool SteeringIsFace(Elite::Blackboard* pBlackboard)
{
	ISteeringBehavior** pSteering = nullptr;
	Face* pFace = nullptr;

	pBlackboard->GetData("SteeringBehavior", pSteering);
	pBlackboard->GetData("Face", pFace);

	return *pSteering == pFace;
}

bool SteeringOnCooldown(Elite::Blackboard* pBlackboard)
{
	float SteeringCooldownRemaining{};
	pBlackboard->GetData("SteeringCooldownRemaining", SteeringCooldownRemaining);

	return SteeringCooldownRemaining > 0.f;
}

//-----------------------------------------------------------------
// Behavior Conditionals
//-----------------------------------------------------------------

bool isHouseInFOV(Elite::Blackboard* pBlackboard)
{
	std::vector<HouseInfo> houses{};
	AgentInfo agentInfo{};
	std::vector<LastSeen>* enteredHouses{};
	pBlackboard->GetData("Houses", houses);
	pBlackboard->GetData("Agent", agentInfo);
	pBlackboard->GetData("EnteredHouses", enteredHouses);
	if (houses.size() <= 0) return false;

	float justEnteredMargin{ 5.f }; //In case agent entered but but influenced out right after
	houses.erase(std::remove_if(houses.begin(), houses.end(), [&enteredHouses, justEnteredMargin](const HouseInfo& house) {
		return std::find_if(enteredHouses->begin(), enteredHouses->end(), [&house, &justEnteredMargin](const LastSeen& lastSeen) {
			return house.Center == lastSeen.SeenLocation && lastSeen.timeElapsed > justEnteredMargin;
			}) != enteredHouses->end();
	}), houses.end());

	if (houses.size() == 0) return false;

	TargetData target{};
	auto nearestHouse = std::min_element(houses.begin(), houses.end(), 
		[&agentInfo](const HouseInfo& left, const HouseInfo& right){
			return Elite::DistanceSquared(left.Center, agentInfo.Position) < Elite::DistanceSquared(right.Center, agentInfo.Position);
		});
	target.Position = (*nearestHouse).Center;

	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);
	pBlackboard->ChangeData("LocationToCheckOut", target);
	return true;

}

bool isEnemyInFOV(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);

	
	//Remove non enemies, transform entities to actual enemies
	std::vector<EnemyInfo> enemies{};
	GetEnemiesInFOV(entities, enemies, pInterface);

	if (enemies.size() <= 0) return false;

	TargetData target{};
	Elite::Vector2 direction{};
	for (const EnemyInfo& enemy : enemies)
	{
		float distance{ Elite::Distance(enemy.Location, agent.Position) };
		float weight{ 1 - (distance / agent.FOV_Range) };

		Elite::Vector2 currDir{ enemy.Location - agent.Position };
		currDir.Normalize();
		direction += weight * currDir;

		AddEnemySeenLocation(pBlackboard, enemy);
	}
	direction.Normalize();

	target.Position = agent.Position + direction;
	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);

	return true;
}

bool isItemInFOV(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);

	std::vector<ItemInfo> items{};
	GetItemsInFOV(entities, items, pInterface);

	if (items.size() <= 0) return false;

	//TODO: choose which item to get
	TargetData target{};
	target.Position = items[0].Location;
	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);

	return true;
}

bool isItemInRange(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);


	std::vector<ItemInfo> items{};
	GetItemsInFOV(entities, items, pInterface);

	float grabRangeSqrd{ agent.GrabRange * agent.GrabRange };
	for (const ItemInfo& item : items)
	{
		float distanceItemAgentSqrd{ Elite::DistanceSquared(agent.Position, item.Location) };
		if (distanceItemAgentSqrd <= grabRangeSqrd) return true;
	}

	return false;
}

bool isPurgeZoneInFOV(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return false;

	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);


	//Remove non enemies, transform entities to actual enemies
	std::vector<PurgeZoneInfo> purgeZones{};
	GetPurgeZonesInFOV(entities, purgeZones, pInterface);

	if (purgeZones.size() == 0) return false;

	TargetData target{};
	target.Position = purgeZones[0].Center;
	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);
	pBlackboard->ChangeData("SteeringCooldownRemaining", 0.f);

	return true;
}

float GetWeight(const AgentInfo& agentInfo, const LastSeen& enemyLastSeen, float memoryFleeRange, float memoryTime)
{
	float distance{ Elite::Distance(enemyLastSeen.SeenLocation, agentInfo.Position) };
	float distanceWeight{ 1 - distance / memoryFleeRange };
	float timeWeight{ 1 - enemyLastSeen.timeElapsed / memoryTime };

	return distanceWeight * timeWeight;
}

bool remembersEnemies(Elite::Blackboard* pBlackboard)
{
	std::vector<LastSeen>* pEnemiesLastSeen = nullptr;
	pBlackboard->GetData("EnemiesLastSeen", pEnemiesLastSeen);
	
	if (pEnemiesLastSeen->size() <= 0) return false;
	
	float enemyMemoryTime{};
	pBlackboard->GetData("EnemyMemoryTime", enemyMemoryTime);
	float rememberedEnemyFleeRange{};
	pBlackboard->GetData("RememberedEnemyFleeRange", rememberedEnemyFleeRange);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	TargetData target{};
	pBlackboard->GetData("Target", target);

	Elite::Vector2 evadePosition{ pEnemiesLastSeen->begin()->PredictedLocation };
	float accumulatedWeight{ GetWeight(agent, *pEnemiesLastSeen->begin(), rememberedEnemyFleeRange, enemyMemoryTime) };
	if (pEnemiesLastSeen->size() > 1)
	{
		std::for_each(pEnemiesLastSeen->begin() + 1, pEnemiesLastSeen->end(), 
			[&agent, rememberedEnemyFleeRange, enemyMemoryTime, &evadePosition, &accumulatedWeight](const LastSeen& enemyLastSeen) {
				float weight{ GetWeight(agent, enemyLastSeen, rememberedEnemyFleeRange, enemyMemoryTime) };
				Elite::Vector2 directionToLastSeen{ enemyLastSeen.PredictedLocation - evadePosition };
				evadePosition += weight * directionToLastSeen.Magnitude() * Elite::GetNormalized(directionToLastSeen);
				accumulatedWeight += weight;
		});
	}

	pBlackboard->ChangeData("RememberFleeLocation", evadePosition);
	pBlackboard->ChangeData("RememberFleeLocationWeight", accumulatedWeight);

	return true;
}

bool remembersLocationToCheckOut(Elite::Blackboard* pBlackboard)
{
	TargetData locationToCheckOut{};
	pBlackboard->GetData("LocationToCheckOut", locationToCheckOut);

	if (Elite::AreEqual(locationToCheckOut.Position.x, 0.f) && Elite::AreEqual(locationToCheckOut.Position.y, 0.f)) return false;

	pBlackboard->ChangeData("Target", locationToCheckOut);
	pBlackboard->ChangeData("IntermediateTarget", locationToCheckOut);
	return true;
}
/// 
/// Behavior Actions
/// 
BehaviorState RunFromDamagingEnemy(Elite::Blackboard* pBlackboard)
{
	Elite::Vector2 probableEnemyLocation{};
	std::vector<LastSeen>* pLastSeenEnemies = nullptr;
	AgentInfo agent{};
	pBlackboard->GetData("RememberFleeLocation", probableEnemyLocation);
	pBlackboard->GetData("EnemiesLastSeen", pLastSeenEnemies);
	pBlackboard->GetData("Agent", agent);

	if (pLastSeenEnemies == nullptr) return Failure;

	Elite::Vector2 probableDamageDirection{ probableEnemyLocation - agent.Position };
	Elite::Normalize(probableDamageDirection);
	Elite::Vector2 probableDamageLocation{ agent.Position + probableDamageDirection };

	pLastSeenEnemies->push_back(LastSeen{ probableDamageLocation });

	return Success;
}

BehaviorState StartRunning(Elite::Blackboard* pBlackboard)
{
	pBlackboard->ChangeData("RunMode", true);

	return Success;
}

BehaviorState StopRunning(Elite::Blackboard* pBlackboard)
{
	pBlackboard->ChangeData("RunMode", false);
	return Success;
}

BehaviorState ChangeSteering(Elite::Blackboard* pBlackboard, ISteeringBehavior* pNewBehavior)
{
	ISteeringBehavior** pSteering = nullptr;
	float SteeringCooldownRemaining{};
	pBlackboard->GetData("SteeringBehavior", pSteering);
	pBlackboard->GetData("SteeringCooldownRemaining", SteeringCooldownRemaining);

	if (pSteering == nullptr || pNewBehavior == nullptr || SteeringCooldownRemaining > 0.f) return Failure;

	float SteeringCooldown{}, variableSteeringCooldown{};
	pBlackboard->GetData("SteeringCooldown", SteeringCooldown);
	pBlackboard->GetData("VariableSteeringCooldown", variableSteeringCooldown);

	*pSteering = pNewBehavior;
	if (variableSteeringCooldown > 0.f)
	{
		pBlackboard->ChangeData("SteeringCooldownRemaining", variableSteeringCooldown);
		pBlackboard->ChangeData("VariableSteeringCooldown", 0.f);
	}

	return Success;
}

BehaviorState ChangeToSeek(Elite::Blackboard* pBlackboard)
{
	Seek* pSeek = nullptr;
	pBlackboard->GetData("Seek", pSeek);

	if (pSeek == nullptr) return Failure;

	return ChangeSteering(pBlackboard, pSeek);
}

BehaviorState ChangeToWander(Elite::Blackboard* pBlackboard)
{
	ISteeringBehavior** pSteering = nullptr;
	Wander* pWander = nullptr;
	AgentInfo agent{};
	pBlackboard->GetData("SteeringBehavior", pSteering);
	pBlackboard->GetData("Wander", pWander);
	pBlackboard->GetData("Agent", agent);

	if (pSteering == nullptr || pWander == nullptr) return Failure;

	if(*pSteering != pWander)
		pWander->SetWanderAngle(agent.Orientation);

	return ChangeSteering(pBlackboard, pWander);
}

BehaviorState ChangeToFlee(Elite::Blackboard* pBlackboard)
{
	Flee* pFlee = nullptr;
	pBlackboard->GetData("Flee", pFlee);

	if (pFlee == nullptr) return Failure;

	return ChangeSteering(pBlackboard, pFlee);
}

BehaviorState ChangeToFace(Elite::Blackboard* pBlackboard)
{
	Face* pFace = nullptr;
	pBlackboard->GetData("Face", pFace);

	if (pFace == nullptr) return Failure;


	return ChangeSteering(pBlackboard, pFace);
}

BehaviorState UpdateTargetWithEnemyMemory(Elite::Blackboard* pBlackboard)
{
	ISteeringBehavior** pSteering = nullptr;
	Seek* pSeek = nullptr;
	Flee* pFlee = nullptr;
	Wander* pWander = nullptr;
	Face* pFace = nullptr;
	TargetData intermediateTarget{};
	Elite::Vector2 rememberedFleeLocation{};
	float rememberedFleeLocationWeight{};

	pBlackboard->GetData("SteeringBehavior", pSteering);
	pBlackboard->GetData("Seek", pSeek);
	pBlackboard->GetData("Flee", pFlee);
	pBlackboard->GetData("Wander", pWander);
	pBlackboard->GetData("IntermediateTarget", intermediateTarget);
	pBlackboard->GetData("RememberFleeLocation", rememberedFleeLocation);
	pBlackboard->GetData("RememberFleeLocationWeight", rememberedFleeLocationWeight);

	if (pSteering == nullptr || pSeek == nullptr || pFlee == nullptr || pWander == nullptr || pFace == nullptr || *pSteering == pFace) return Failure;

	if (*pSteering == pFlee)
	{
		Elite::Vector2 fleeTargetToRememberedFleeLocation{ rememberedFleeLocation - intermediateTarget.Position };
		intermediateTarget.Position += rememberedFleeLocationWeight * fleeTargetToRememberedFleeLocation.Magnitude() * Elite::GetNormalized(fleeTargetToRememberedFleeLocation);
		pBlackboard->ChangeData("Target", intermediateTarget);
		return Success;
	}
	if (*pSteering == pWander)
	{
		TargetData Td{};
		Td.Position = rememberedFleeLocation;
		pBlackboard->ChangeData("Target", Td);
		return ChangeToFlee(pBlackboard);
	}
	if (*pSteering == pSeek)
	{
		AgentInfo agent{};
		pBlackboard->GetData("Agent", agent);
		Elite::Vector2 agentToSeekTarget{ intermediateTarget.Position - agent.Position };
		Elite::Vector2 agentToRememberedFleeLocation{ rememberedFleeLocation - agent.Position };
		bool isSeekingTowardsRememberedEnemies{ Elite::Dot(agentToSeekTarget, agentToRememberedFleeLocation) > 0 };

		//If agent is not seeking towards remembered enemies, don't interfere
		if (!isSeekingTowardsRememberedEnemies) return Success;

		//Else agent is seeking towards enemies -> Update seek position
		Elite::Vector2 rememberedFleeLocationToSeekTarget{ intermediateTarget.Position - rememberedFleeLocation };
		intermediateTarget.Position += rememberedFleeLocationToSeekTarget;
		pBlackboard->ChangeData("Target", intermediateTarget);
		return Success;
	}

	//This line should never be reached
	return Failure;
}

BehaviorState ExitHouse(Elite::Blackboard* pBlackboard)
{
	Elite::Vector2 houseEnteredAt{};
	pBlackboard->GetData("HouseEnteredAt", houseEnteredAt);

	TargetData target{};
	target.Position = houseEnteredAt;
	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);
	return ChangeToSeek(pBlackboard);
}

BehaviorState FaceEnemy(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return Failure;

	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);

	//Remove non enemies, transform entities to actual enemies
	std::vector<EnemyInfo> enemies{};
	GetEnemiesInFOV(entities, enemies, pInterface);

	if (enemies.size() == 0) return Failure;

	auto nearestEnemy = std::min_element(enemies.begin(), enemies.end(), [&agent](const EnemyInfo& left, const EnemyInfo& right){
		return Elite::DistanceSquared(left.Location, agent.Position) < Elite::DistanceSquared(right.Location, agent.Position);
	});

	TargetData target{};
	target.Position = nearestEnemy->Location;
	pBlackboard->ChangeData("Target", target);

	return ChangeToFace(pBlackboard);
}

BehaviorState TurnAround(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	Elite::Vector2 lookAtPos{ agent.Position - agent.LinearVelocity.GetNormalized() * 5.f };

	TargetData target{};
	target.Position = lookAtPos;
	pBlackboard->ChangeData("Target", target);

	const float faceSteeringCooldown{ 2.f };
	pBlackboard->ChangeData("VariableSteeringCooldown", faceSteeringCooldown);

	std::cout << "Turn Around" << '\n';

	return ChangeToFace(pBlackboard);
}

BehaviorState UpdateWorldPath(Elite::Blackboard* pBlackboard)
{
	AgentInfo agent{};
	std::vector<Elite::Vector2>* pPath = nullptr;
	size_t* pCurrentPathNode = nullptr;
	pBlackboard->GetData("Path", pPath);
	pBlackboard->GetData("CurrentPathNode", pCurrentPathNode);
	pBlackboard->GetData("Agent", agent);

	if (pPath == nullptr || pCurrentPathNode == nullptr) return Failure;

	Elite::Vector2 currentNode{ pPath->at(*pCurrentPathNode) };
	const float closeEnoughRange{ 10.f };
	const float closeEnoughSqrd{ closeEnoughRange * closeEnoughRange };

	if (Elite::DistanceSquared(agent.Position, currentNode) <= closeEnoughSqrd)
	{
		(*pCurrentPathNode)++;
		*pCurrentPathNode %= pPath->size();
		currentNode = pPath->at(*pCurrentPathNode);
	}

	TargetData target{};
	target.Position = currentNode;
	pBlackboard->ChangeData("Target", target);
	pBlackboard->ChangeData("IntermediateTarget", target);

	return ChangeToSeek(pBlackboard);
}

//Items Actions
BehaviorState GrabItem(Elite::Blackboard* pBlackboard)
{
	std::vector<EntityInfo> entities{};
	pBlackboard->GetData("Entities", entities);
	if (entities.size() <= 0) return Failure;

	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	GetEntitiesOfTypeInFOV(entities, eEntityType::ITEM, pInterface);
	if (entities.size() <= 0) return Failure;

	Inventory* pInventory = nullptr;
	pBlackboard->GetData("Inventory", pInventory);

	float grabRangeSqrd{ agent.GrabRange * agent.GrabRange };
	//Remove all items not in range
	entities.erase(std::remove_if(entities.begin(), entities.end(), [&grabRangeSqrd, &agent](const EntityInfo& item) {
		float distanceAgentItemSqrd{ Elite::DistanceSquared(item.Location, agent.Position) };
		return distanceAgentItemSqrd > grabRangeSqrd;
	}), entities.end());
	if (entities.size() <= 0) return Failure;

	for (const EntityInfo& entity : entities)
	{
		int amountOfItemsInInventory{ pInventory->GetAmountOfItemsInInventory() };
		UINT capacity{ pInterface->Inventory_GetCapacity() };
		ItemInfo itemInfo{};
		pInterface->Item_GetInfo(entity, itemInfo);

		//If it's garbage, just destroy it
		if (itemInfo.Type == eItemType::GARBAGE)
		{
			pInterface->Item_Destroy(entity);
			continue;
		}

		//If we still have space, pick it up for later
		if (amountOfItemsInInventory < int(capacity))
		{
			int itemSlot = pInventory->GetFirstEmptySpace();
			pInventory->GrabItem(itemSlot, entity);
			continue;
		}

		//No space available -> Use item in inventory to make space
		int amountFoodHeld{ pInventory->GetAmountOfItemsHeldOfType(eItemType::FOOD) };
		int amountMedkitHeld{ pInventory->GetAmountOfItemsHeldOfType(eItemType::MEDKIT) };
		int amountPistolHeld{ pInventory->GetAmountOfItemsHeldOfType(eItemType::PISTOL) };

		int max{ std::max<int>(amountFoodHeld, std::max<int>(amountMedkitHeld, amountPistolHeld)) };
		//If it's food or medkit, use it before picking up the other thing
		if (max == amountFoodHeld || max == amountMedkitHeld)
		{
			int itemSlot = 0; 
			pInventory->GetHealthpack(itemSlot, agent.Health, true);
			//Prefer dropping food over medkit (in case holding equal amount of food and medkits)
			if (max == amountFoodHeld || itemInfo.Type == eItemType::FOOD) pInventory->GetFood(itemSlot, agent.Energy, true);
			pInventory->UseItem(itemSlot);
			pInventory->GrabItem(itemSlot, entity);
			continue;
		}

		//Amount of pistols held is highest
		int gunSlot{0};
		pInventory->GetPistol(gunSlot);
		//Get gun and its ammo
		ItemInfo gun{};
		pInterface->Inventory_GetItem(gunSlot, gun);
		
		pInventory->RemoveItem(gunSlot);
		pInventory->GrabItem(gunSlot, entity);
	}

	return Success;
}

BehaviorState ShootPistol(Elite::Blackboard* pBlackboard)
{
	IExamInterface* pInterface = nullptr;
	pBlackboard->GetData("Interface", pInterface);
	Inventory* pInventory = nullptr;
	pBlackboard->GetData("Inventory", pInventory);

	int gunSlot{ -1 };
	pInventory->GetPistol(gunSlot);
	if (gunSlot < 0) return Failure;

	pInventory->UseItem(gunSlot);
	
	return Success;
}

BehaviorState RestoreHealth(Elite::Blackboard* pBlackboard)
{
	Inventory* pInventory = nullptr;
	pBlackboard->GetData("Inventory", pInventory);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	int medkitSlot{ -1 };
	pInventory->GetHealthpack(medkitSlot, agent.Health);
	if (medkitSlot < 0) return Failure;

	pInventory->UseItem(medkitSlot);
	return Success;
}

BehaviorState RestoreEnergy(Elite::Blackboard* pBlackboard)
{
	Inventory* pInventory = nullptr;
	pBlackboard->GetData("Inventory", pInventory);
	AgentInfo agent{};
	pBlackboard->GetData("Agent", agent);

	int foodSlot{ -1 };
	pInventory->GetFood(foodSlot, agent.Energy);
	if (foodSlot < 0) return Failure;

	pInventory->UseItem(foodSlot);
	return Success;
}

BehaviorState DebugPrint(Elite::Blackboard* pBlckboard)
{
	std::cout << "Debug Activate" << '\n';

	return Success;
}
#endif