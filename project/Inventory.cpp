#include "stdafx.h"
#include "Inventory.h"
#include "IExamInterface.h"

int Inventory::m_AmountOfItems{ 5 };

Inventory::Inventory(IExamInterface* pInterface)
	: m_pInterface{pInterface}
{
	m_AmountOfItems = m_pInterface->Inventory_GetCapacity();
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		ItemSlot itemSlot{};
		itemSlot.id = i;
		itemSlot.inUse = false;
		m_Items.push_back(itemSlot);
	}
}

void Inventory::GrabItem(int id, const EntityInfo& entityInfo)
{
	m_Items[id].inUse = true;
	ItemInfo itemInfo{};
	m_pInterface->Item_Grab(entityInfo, itemInfo);
	m_Items[id].itemType = itemInfo.Type;
	m_pInterface->Inventory_AddItem(UINT(id), itemInfo);
}

void Inventory::UseItem(int id)
{
	m_pInterface->Inventory_UseItem(UINT(id));
	switch (m_Items[id].itemType)
	{
	case eItemType::FOOD:
	case eItemType::MEDKIT:
		m_pInterface->Inventory_RemoveItem(UINT(id));
		m_Items[id].inUse = false;
		break;
	case eItemType::PISTOL:
		ItemInfo gun{};
		m_pInterface->Inventory_GetItem(UINT(id), gun);
		int ammo = m_pInterface->Weapon_GetAmmo(gun);
		if (ammo <= 0)
		{
			m_pInterface->Inventory_RemoveItem(UINT(id));
			m_Items[id].inUse = false;
		}
		break;
	}
	
}

void Inventory::RemoveItem(int id)
{
	if (!m_Items[id].inUse) return;

	m_pInterface->Inventory_RemoveItem(UINT(id));
	m_Items[id].inUse = false;
}

int Inventory::GetAmountOfItemsInInventory() const
{
	int filledSpaces{};
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (m_Items[i].inUse) filledSpaces++;
	}
	return filledSpaces;
}

int Inventory::GetFirstEmptySpace() const
{
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (!m_Items[i].inUse) return i;
	}

	return -1;
}

int Inventory::GetAmountOfItemsHeldOfType(const eItemType& itemType) const
{
	int amountOfItemsHeld{};
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (m_Items[i].inUse && m_Items[i].itemType == itemType) amountOfItemsHeld++;
	}
	return amountOfItemsHeld;
}

bool Inventory::GetHealthpack(int& slotId, float agentHealth, bool ignoreAgentHealth) const
{
	int lowestHealthRestore = INT_MAX;
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (m_Items[i].inUse && m_Items[i].itemType == eItemType::MEDKIT)
		{
			ItemInfo medkit{};
			m_pInterface->Inventory_GetItem(UINT(i), medkit);
			int healthRestore = m_pInterface->Medkit_GetHealth(medkit);
			if (healthRestore < lowestHealthRestore && (healthRestore + agentHealth <= 10.f || ignoreAgentHealth))
			{
				lowestHealthRestore = healthRestore;
				slotId = i;
			}
		}
	}

	return slotId >= 0 && slotId <= m_AmountOfItems;
}

bool Inventory::GetFood(int& slotId, float agentFood, bool ignoreAgentEnergy) const
{
	int lowestFoodRestore = INT_MAX;
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (m_Items[i].inUse && m_Items[i].itemType == eItemType::FOOD)
		{
			ItemInfo food{};
			m_pInterface->Inventory_GetItem(UINT(i), food);
			int foodRestore = m_pInterface->Food_GetEnergy(food);
			if (foodRestore < lowestFoodRestore && (foodRestore + agentFood <= 10.f || ignoreAgentEnergy))
			{
				lowestFoodRestore = foodRestore;
				slotId = i;
			}
		}
	}

	return slotId >= 0 && slotId <= m_AmountOfItems;
}

bool Inventory::GetPistol(int& slotId) const
{
	int lowestAmmoPistol = INT_MAX;
	for (int i = 0; i < m_AmountOfItems; i++)
	{
		if (m_Items[i].inUse && m_Items[i].itemType == eItemType::PISTOL)
		{
			ItemInfo pistol{};
			m_pInterface->Inventory_GetItem(UINT(i), pistol);
			int ammo = m_pInterface->Weapon_GetAmmo(pistol);
			if (ammo < lowestAmmoPistol)
			{
				lowestAmmoPistol = ammo;
				slotId = i;
			}
		}
	}

	return slotId >= 0 && slotId <= m_AmountOfItems;
}
