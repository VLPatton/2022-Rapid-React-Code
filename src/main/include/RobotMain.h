/******************************************************************************
    Description:	CRobotMain implementation
	Classes:		CRobotMain
	Project:		2022 Rapid React Robot Code
******************************************************************************/

#ifndef RobotMain_h
#define RobotMain_h

#include "Drive.h"
#include "Intake.h"
#include "Shooter.h"
#include "Lift.h"

#include <string>
#include <frc/TimedRobot.h>
#include <frc/Timer.h>
#include <frc/smartdashboard/SendableChooser.h>
///////////////////////////////////////////////////////////////////////////////
using namespace frc;

class CRobotMain : public TimedRobot {
public:
	CRobotMain();
	~CRobotMain();
	void RobotInit() override;
	void RobotPeriodic() override;
	void AutonomousInit() override;
	void AutonomousPeriodic() override;
	void TeleopInit() override;
	void TeleopPeriodic() override;
	void DisabledInit() override;
	void DisabledPeriodic() override;
	void TestInit() override;
	void TestPeriodic() override;

private:
	enum AutoStates {
		eAutoStopped = 0,
		eAutoIdle,

	};

	enum TeleopStates {
		eTeleopStopped = 0,
		eTeleopIdle,
		eTeleopFindingBall,
		eTeleopHomingBall,
		eTeleopClimbing,
		eTeleopIntake,
		eTeleopFiring
	};

	frc::SendableChooser<std::string>*	m_pAutoChooser;
	std::string							m_strAutoSelected;
	Joystick*							m_pDriveController;
	Joystick*							m_pAuxController;
	CDrive*								m_pDrive;
	Timer*								m_pTimer;
	CIntake*							m_pIntake;
	CShooter*							m_pShooter;

	double	m_dStartTime;
	int		m_nAutoState;
	int		m_nTeleopState;
	int		m_nPreviousState;
};
#endif