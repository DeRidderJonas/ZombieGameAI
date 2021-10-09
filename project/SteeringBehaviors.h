/*=============================================================================*/
// Copyright 2017-2018 Elite Engine
// Authors: Matthieu Delaere, Thomas Goussaert
/*=============================================================================*/
// SteeringBehaviors.h: SteeringBehaviors interface and different implementations
/*=============================================================================*/
#ifndef ELITE_STEERINGBEHAVIORS
#define ELITE_STEERINGBEHAVIORS

//-----------------------------------------------------------------
// Includes & Forward Declarations
//-----------------------------------------------------------------
#include <Exam_HelperStructs.h>
#include "SteeringHelpers.h"

using namespace Elite;

#pragma region **ISTEERINGBEHAVIOR** (BASE)
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	virtual SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) = 0;

	//Seek Functions
	void SetTarget(const TargetData& target) { m_Target = target; }

	template<class T, typename std::enable_if<std::is_base_of<ISteeringBehavior, T>::value>::type* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	TargetData m_Target;
};
#pragma endregion

///////////////////////////////////////
//SEEK
//****
class Seek : public ISteeringBehavior
{
public:
	Seek() = default;
	virtual ~Seek() = default;

	//Seek Behaviour
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;
};

//////////////////////////
//WANDER
//******
class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() = default;

	//Wander Behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

	void SetWanderOffset(float offset) { m_Offset = offset; };
	void SetMaxAngleChange(float rad) { m_AngleChange = rad; };
	void SetWanderAngle(float currentAngle);
protected:
	float m_Offset = 6.f; //Offset for target
	float m_AngleChange = ToRadians(15); //max angle change per frame
	float m_WanderAngle = 0.f; //curr Angle
};


///////////////////////////
//FLEE
//****
class Flee : public Seek
{
public:
	Flee() = default;
	virtual ~Flee() = default;

	//Flee behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;
};

//////////////////////////
//ARRIVE
//******
class Arrive : public Seek
{
public:
	Arrive() = default;
	virtual ~Arrive() = default;

	//Arrive behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

	void SetSlowRadius(float slowRadius);
	void SetTargetRadius(float targetRadius);
private:
	float m_ArrivalRadius = 1.f;
	float m_SlowRadius = 20.f;
};


//////////////////////////
//FACE
//****
class Face : public Seek
{
public:
	Face() = default;
	virtual ~Face() = default;

	//Flee behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;
};


//////////////////////////
//EVADE
//*****
class Evade : public Flee
{
public:
	Evade() = default;
	virtual ~Evade() = default;

	void SetEvadeRadius(float radius);
	float GetEvadeRadius() const;

	//Evade behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;

private:
	float m_EvadeRadius = 20.f;
};


//////////////////////////
//PURSUIT
//*******
class Pursuit : public Seek
{
public:
	Pursuit() = default;
	virtual ~Pursuit() = default;

	//Pursuit behavior
	SteeringPlugin_Output CalculateSteering(float deltaT, const AgentInfo& agentInfo) override;
};
#endif


