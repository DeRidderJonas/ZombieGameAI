#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "HelperStructs.h"
#include "SteeringBehaviors.h"
#include "EBehaviorTree.h"
#include "Inventory.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose

	//Enemy memory
	float m_EnemyMemoryTime = 2.5f; //Amount of seconds positions enemies were last seen at are remembered
	float m_RememberedEnemyFleeRange = 20.f; //After surpassing this value, dont worry about the enemies anymore
	std::vector<LastSeen> m_EnemiesLastSeen{};

	//House memory
	std::vector<LastSeen> m_HousesEntered{};
	float m_HouseMemoryTime = 120.f;

	//Agent memory
	const size_t m_AgentHistorySize{ 50 };
	std::vector<AgentInfo> m_AgentHistory{};
	size_t m_PreviousAgentHistoryIndex{ 0 };

	//Path
	std::vector<Elite::Vector2> m_Path{};
	size_t m_CurrentPathNode{ 0 };

	Elite::Blackboard* m_pBlackboard = nullptr;
	Elite::IDecisionMaking* m_pBehaviorTree = nullptr;

	//Inventory
	Inventory* m_pInventory = nullptr;

	//Steering
	ISteeringBehavior* m_pSteeringBehavior = nullptr; //Will point to current steering behavior, doesn't get initiated. Do not delete
	Seek* m_pSeek = nullptr;
	Wander* m_pWander = nullptr;
	Flee* m_pFlee = nullptr;
	Face* m_pFace = nullptr;
	const float m_SteeringCooldown = 0.5f;
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}