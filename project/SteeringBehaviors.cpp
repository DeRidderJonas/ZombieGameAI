//Precompiled Header [ALWAYS ON TOP IN CPP]
#include "stdafx.h"
#include <iomanip>

//Includes
#include "SteeringBehaviors.h"

//SEEK
//****
SteeringPlugin_Output Seek::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	SteeringPlugin_Output steering{};
	steering.AutoOrient = true;
	steering.LinearVelocity = m_Target.Position - agentInfo.Position;
	steering.LinearVelocity.Normalize();
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed;

	return steering;
}

//WANDER (base> SEEK)
//******
SteeringPlugin_Output Wander::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	TargetData target{};

	Elite::Vector2 currDirection{ m_Target.Position - agentInfo.Position };
	currDirection.Normalize();
	
	float randAngle{ ToRadians(float(rand() % 360)) };
	while (abs(randAngle - abs(m_WanderAngle)) > m_AngleChange) 
	{ 
		randAngle = ToRadians(float(rand() % 360)); 
	};

	m_WanderAngle = randAngle;

	target.Position.x = cos(randAngle);
	target.Position.y = sin(randAngle);
	target.Position *= m_Offset;
	target.Position += agentInfo.Position;

	SetTarget(target);

	return Seek::CalculateSteering(deltaT, agentInfo);
}

void Wander::SetWanderAngle(float currentAngle)
{
	m_WanderAngle = currentAngle;
}

//FLEE (base> SEEK)
//****
SteeringPlugin_Output Flee::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	SteeringPlugin_Output flee{ Seek::CalculateSteering(deltaT, agentInfo) };
	flee.LinearVelocity *= -1;

	return flee;
}

//ARRIVE
//******
SteeringPlugin_Output Arrive::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	SteeringPlugin_Output arrive{};
	arrive.AutoOrient = true;

	arrive.LinearVelocity = m_Target.Position - agentInfo.Position;
	const float distance{ arrive.LinearVelocity.Magnitude() };
	if (distance < m_ArrivalRadius)
	{
		arrive.LinearVelocity = Elite::ZeroVector2;
		return arrive;
	}

	arrive.LinearVelocity.Normalize();
	if (distance < m_SlowRadius) arrive.LinearVelocity *= agentInfo.MaxLinearSpeed * (distance / m_SlowRadius);
	else arrive.LinearVelocity *= agentInfo.MaxLinearSpeed;

	return arrive;
}

void Arrive::SetSlowRadius(float slowRadius)
{
	m_SlowRadius = slowRadius;
}

void Arrive::SetTargetRadius(float targetRadius)
{
	m_ArrivalRadius = targetRadius;
}

//FACE
//****
SteeringPlugin_Output Face::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	SteeringPlugin_Output face{};
	face.AutoOrient = false;

	Elite::Vector2 direction{ m_Target.Position - agentInfo.Position };
	float directionAngle{ Elite::GetOrientationFromVelocity(direction) };

	float currAngle{ Elite::GetOrientationFromVelocity(Elite::OrientationToVector(agentInfo.Orientation)) };

	float angleDiff = directionAngle - currAngle;
	if (directionAngle > 1.5f && currAngle < -1.5f) angleDiff = directionAngle - (currAngle + 6.28f);
	if (directionAngle < -1.5f && currAngle > 1.5f) angleDiff = (directionAngle + 6.28f) - currAngle;

	face.AngularVelocity = (angleDiff > 0 ? 1 : -1) * agentInfo.MaxAngularSpeed;
	if (abs(angleDiff) < 0.1f) face.AngularVelocity *= angleDiff;

	return face;
}

void Evade::SetEvadeRadius(float radius)
{
	m_EvadeRadius = radius;
}

float Evade::GetEvadeRadius() const
{
	return m_EvadeRadius;
}

//EVADE
//*****
SteeringPlugin_Output Evade::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	auto distance{ Distance(agentInfo.Position, m_Target.Position) };
	if (distance > m_EvadeRadius)
	{
		SteeringPlugin_Output steering;
		//steering.IsValid = false;
		return steering;
	}

	m_Target.LinearVelocity.Normalize();

	Elite::Vector2 predPoint{ m_Target.Position + m_Target.LinearVelocity * distance / 2.f };
	TargetData targetData{};
	targetData.Position = predPoint;
	SetTarget(targetData);

	return Flee::CalculateSteering(deltaT, agentInfo);
}

//PURSUIT
//*******
SteeringPlugin_Output Pursuit::CalculateSteering(float deltaT, const AgentInfo& agentInfo)
{
	Elite::Vector2 predPoint{m_Target.Position + m_Target.LinearVelocity};
	
	TargetData targetData{};
	targetData.Position = predPoint;
	SetTarget(targetData);

	return Seek::CalculateSteering(deltaT, agentInfo);
}
