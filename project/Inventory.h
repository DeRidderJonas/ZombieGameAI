#pragma once
#include "Exam_HelperStructs.h"

class IExamInterface;

class Inventory final
{
public:
	Inventory(IExamInterface* pInterface);
	~Inventory() = default;
	Inventory(const Inventory&) = delete;
	Inventory& operator=(const Inventory&) = delete;
	Inventory(Inventory&&) = delete;
	Inventory& operator=(Inventory&&) = delete;

	void GrabItem(int id, const EntityInfo& entityInfo);
	void UseItem(int id);
	void RemoveItem(int id);

	int GetAmountOfItemsInInventory() const;
	int GetFirstEmptySpace() const;
	int GetAmountOfItemsHeldOfType(const eItemType& itemType) const;

	bool GetHealthpack(int& slotId, float agentHealth, bool ignoreAgentHealth = false) const;
	bool GetFood(int& slotId, float agentFood, bool ignoreAgentEnergy = false) const;
	bool GetPistol(int& slotId) const;
private:
	struct ItemSlot
	{
		int id;
		eItemType itemType;
		bool inUse;
	};
	static int m_AmountOfItems;

	IExamInterface* m_pInterface = nullptr;
	std::vector<ItemSlot> m_Items{};
};

