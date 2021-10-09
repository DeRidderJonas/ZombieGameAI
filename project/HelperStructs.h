#pragma once
#include "stdafx.h"

struct LastSeen
{
	Elite::Vector2 SeenLocation;
	Elite::Vector2 PredictedLocation;
	Elite::Vector2 Velocty;
	float timeElapsed;

	LastSeen(const Elite::Vector2& location, const Elite::Vector2& velocity = {})
		: SeenLocation{location}
		, PredictedLocation{}
		, Velocty{velocity}
		, timeElapsed{0.f}
	{}

	static const float sameLocationMargin;

	bool operator==(const LastSeen& other)
	{
		return SeenLocation == other.SeenLocation;
	}

	bool operator==(const EnemyInfo& other)
	{
		return Elite::DistanceSquared(SeenLocation, other.Location) <= (sameLocationMargin * sameLocationMargin);
	}

	bool operator==(const HouseInfo& house)
	{
		return Elite::DistanceSquared(SeenLocation, house.Center) <= (sameLocationMargin * sameLocationMargin);
	}
};

const float LastSeen::sameLocationMargin = 1.f;