/******************************************************************************
    Description:	CRobotMain implementation
	Classes:		CRobotMain
	Project:		2022 Rapid React Robot Code
******************************************************************************/

#include "RobotMain.h"

#include <fmt/core.h>
#include <frc/smartdashboard/SmartDashboard.h>
#include <frc/DriverStation.h>
#include <wpi/StringExtras.h>
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
using namespace frc;
using namespace std;

/******************************************************************************
    Description:	CRobotMain constructor - initialize local variables
	Arguments:		None
	Derived from:	TimedRobot
******************************************************************************/
CRobotMain::CRobotMain()
{
	m_pDriveController			= new Joystick(0);
	m_pAuxController			= new Joystick(1);
	m_pTimer					= new Timer();
	m_pDrive					= new CDrive(m_pDriveController);
	m_pAutoChooser				= new SendableChooser<Paths>();
	m_pLeftLift					= new CLift(false);
	m_pRightLift				= new CLift(true);
	m_pBackIntake				= new CIntake(nIntakeMotor2, nBackIntakeDownLS, nBackIntakeUpLS, nIntakeDeployMotor2, false);
	m_pShooter					= new CShooter();
	m_nAutoState				= eAutoStopped;
	m_dStartTime				= 0.0;
	m_nPreviousState			= eTeleopStopped;
	m_kClimbState				= eNoClimb;
	m_pPrevVisionPacket         = new CVisionPacket();
	m_pTransfer					= new CTransfer();
}

/******************************************************************************
    Description:	CRobotMain destructor - delete local variables
	Arguments:		None
	Derived from:	TimedRobot
******************************************************************************/
CRobotMain::~CRobotMain()
{
	delete m_pDriveController;
	delete m_pAuxController;
	delete m_pDrive;
	delete m_pTimer;
	delete m_pLeftLift;
	delete m_pRightLift;
	delete m_pAutoChooser;
	delete m_pBackIntake;
	delete m_pPrevVisionPacket;
	delete m_pTransfer;

	m_pDriveController	= nullptr;
	m_pAuxController	= nullptr;
	m_pDrive			= nullptr;
	m_pTimer			= nullptr;
	m_pLeftLift			= nullptr;
	m_pRightLift		= nullptr;
	m_pAutoChooser		= nullptr;
	m_pBackIntake		= nullptr;
	m_pPrevVisionPacket = nullptr;
	m_pTransfer			= nullptr;
}

/******************************************************************************
    Description:	Global robot initialization function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::RobotInit()
{
	m_pDrive->Init();
	m_pTransfer->Init();
	
	m_pLeftLift->Init();
	m_pRightLift->Init();

	// Setup autonomous chooser.
	m_pAutoChooser->SetDefaultOption("Autonomous Idle", eAutoIdle);
	m_pAutoChooser->AddOption("Advancement", eAdvancement1);
	m_pAutoChooser->AddOption("Test Path", eTestPath);
	m_pAutoChooser->AddOption("Dumb Taxi", eDumbTaxi);
	m_pAutoChooser->AddOption("Taxi Shot", eTaxiShot);
	// m_pAutoChooser->AddOption("Less Dumb Taxi", eLessDumbTaxi1);
	m_pAutoChooser->AddOption("YOLO Terminator", eTerminator);
	SmartDashboard::PutData(m_pAutoChooser);

	SmartDashboard::PutBoolean("bTeleopVision", false);

	m_pTimer->Start();
}

/******************************************************************************
    Description:	Global robot 20ms periodic function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::RobotPeriodic()
{
	// Tick the shooter
	m_pShooter->Tick();

	// Tick the transfer system
	m_pTransfer->UpdateLocations();

	// Update SmartDashboard for easy checking.
	SmartDashboard::PutBoolean("Vertical Transfer Infrared", m_pTransfer->m_aBallLocations[0]);
	SmartDashboard::PutBoolean("Back Transfer Infrared", m_pTransfer->m_aBallLocations[1]);
	SmartDashboard::PutBoolean("Back-Down Limit Switch", m_pBackIntake->GetLimitSwitchState(false));
	SmartDashboard::PutBoolean("Back-Up Limit Switch", m_pBackIntake->GetLimitSwitchState(true));
}

/******************************************************************************
    Description:	Autonomous initialization function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::AutonomousInit()
{
	// Init Drive and disable joystick
	m_pDrive->Init();
	m_pDrive->SetDriveSafety(false);
	m_pDrive->SetJoystickControl(false);

	// Record start time
	m_dStartTime = -1;

	// Get selected option and switch m_nAutoState based on that
	m_nAutoState = m_pAutoChooser->GetSelected();
	m_pDrive->SetTrajectory(m_nAutoState);
	
	if(m_nAutoState == eTerminator) {
		m_pBackIntake->ToggleIntake();
	}
}

/******************************************************************************
    Description:	Autonomous 20ms periodic function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::AutonomousPeriodic() 
{
	switch (m_nAutoState) 
	{
		// Force stop everything
		case eAutoStopped:
			m_pDrive->ForceStop();
			m_nPreviousState = eAutoStopped;
			break;

		// Do nothing
		case eAutoIdle:
			switch (m_nPreviousState)
			{
				case eAdvancement1:
					m_nAutoState = eAdvancement2;
					break;
				case eTestPath:
					m_nAutoState = eAutoStopped;
					break;
				default:
					m_nAutoState = eAutoIdle;
					break;
			}
			break;
		
		// Test Path
		case eTestPath:
			m_pDrive->FollowTrajectory();
			if (m_pDrive->IsTrajectoryFinished()) 
			{
				m_nPreviousState = eTestPath;
				m_nAutoState = eAutoIdle;
			}
			break;

		// Dumb Taxi
		case eDumbTaxi:
			if(m_dStartTime < 0) m_dStartTime = (double)m_pTimer->Get();
			
			if( ((double)m_pTimer->Get() - m_dStartTime) < 1.250) 
				m_pDrive->SetDriveSpeeds(-6.000, -6.000);
			else 
				m_nAutoState = eAutoStopped;
			break;

		case eTaxiShot:
			if(m_dStartTime < 0) {
				m_dStartTime = (double)m_pTimer->Get();
			}
			// Drive backwards for 1.25s
			if( ((double)m_pTimer->Get() - m_dStartTime) < 1.250) {
				m_pDrive->SetDriveSpeeds(-6.000, -6.000);
			}
			// Rev up the shooter for shooting
			else {
				m_pDrive->ForceStop();
				
				m_pShooter->StartFlywheelShot();
				// If we don't have a ball in the vertical anymore, shut off the shooter.
				if(!m_pTransfer->m_aBallLocations[0]) {
					m_pShooter->IdleStop();
					m_pTransfer->StopVertical();
					m_nAutoState = eAutoStopped;
				}
				// If the shooter is at full speed, start our vertical shot.
				if(m_pShooter->m_bShooterFullSpeed) {
					m_pTransfer->StartVerticalShot();
				}
			}
			break;
		// Less Dumb Taxi
		case eLessDumbTaxi1:
			if( ((double)m_pTimer->Get() - m_dStartTime) < 0.750) {
				// Drop the intake down and then start it.
				m_pBackIntake->ToggleIntake();
				m_pBackIntake->StartIntake();
				// Drive backwards for a bit
				m_pDrive->SetDriveSpeeds(-6.000, -6.000);
				
				// Start/Stop the back transfer depending on if we've picked up the ball already.
				if(!m_pTransfer->m_aBallLocations[1]) m_pTransfer->StartBack();
				else m_pTransfer->StopBack();
			}
			else {
				m_nPreviousState = eLessDumbTaxi1;
				m_nAutoState = eLessDumbTaxi2;
				// Stop the intake to avoid picking up more than 2 balls
				m_pBackIntake->StopIntake();
			}
			break;
		case eLessDumbTaxi2:
			if( ((double)m_pTimer->Get() - m_dStartTime) < 1.750) {
				// Drive backwards more
				m_pDrive->SetDriveSpeeds(-6.000, -6.000);
			}
			else {
				m_nPreviousState = eLessDumbTaxi2;
				m_nAutoState = eLessDumbTaxi3;
			}
			break;
		case eLessDumbTaxi3:
			if(!m_pShooter->m_bShooterFullSpeed && m_pTransfer->m_aBallLocations[0]) {
				// Start the flywheel since the shooter isn't full speed yet.
				m_pShooter->StartFlywheelShot();
			}
			// The flywheel has spun up to full speed, now we start the vertical transfer to feed the ball in.
			if(m_pShooter->m_bShooterFullSpeed) {
				m_pTransfer->StartVerticalShot();
				if (!m_pTransfer->m_aBallLocations[0]) {
					// Spin the vertical transfer to "full" speed just to make sure the back ball is ready.
					m_pTransfer->StartVerticalShot();
					// We need to feed the balls in the horizontal into the vertical and let them sit there.
					m_pTransfer->StartBack();
				}
			}
			// We don't have any balls in the robot, time to stop the autonomous.
			if(!m_pTransfer->m_aBallLocations[0] && !m_pTransfer->m_aBallLocations[1]) {
				m_nAutoState = eAutoStopped;
			}
			break;
	
		case eTerminator:
			// Deploy the intake (similar logic to Teleop)
			if (m_pBackIntake->IsGoalPressed())
			{
				if (!m_pBackIntake->m_bGoal) m_pBackIntake->StartIntake();
				else m_pBackIntake->StopIntake();
				
				m_pBackIntake->StopDeploy();
				m_pBackIntake->m_bGoal = !m_pBackIntake->m_bGoal;
			}
			/*
			// Get the vision packet from the coprocessor.
			CVisionPacket* pVisionPacket = CVisionPacket::GetReceivedPacket();
			if(pVisionPacket != nullptr) {
				if(m_pPrevVisionPacket->m_nRandVal != pVisionPacket->m_nRandVal) {
					pVisionPacket->ParseDetections();
					DetectionClass kDetectClass = (DriverStation::GetAlliance() == DriverStation::Alliance::kBlue ? eBlueCargo : eRedCargo);
					DetectionClass kIgnoreClass = (DriverStation::GetAlliance() == DriverStation::Alliance::kBlue ? eRedCargo  : eBlueCargo);
					
					// Get the amount of balls inside the robot
					int iBallCount = 0;
					for(int i = 0; i < 3; i++) {
						if(m_pTransfer->m_aBallLocations[i]) iBallCount += 1;
					}
					bool bBallHunting = false;
					for(int i = 0; i < pVisionPacket->m_nDetectionCount; i++) {
						CVisionPacket::sObjectDetection* pObjDetection = pVisionPacket->m_pDetections[i];
						
						// Go ball hunting if we don't have 2 balls.
						if(iBallCount < 2) {
							// Check if we detected any balls (luckily the detection information is sorted by depth on the Pi)
							if(pObjDetection->m_kClass == kDetectClass) {
								double dTheta = (pObjDetection->m_nX - 160) * dAnglePerPixel;
								const double dHalfWidthAngle = (pObjDetection->m_nWidth / 2) * dAnglePerPixel;

								// Go forward and head towards the ball
								if (-dHalfWidthAngle < dTheta && dTheta < dHalfWidthAngle) {
									if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eFrontCamera) 
										m_pDrive->SetDriveSpeeds(6.000, 6.000); 
									else if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eBackCamera)
										m_pDrive->SetDriveSpeeds(-6.000, -6.000);
								}
								// Turn to center the detected object
								else {
									dTheta = dTheta * (pVisionPacket->m_kDetectionLocation == DetectionLocation::eFrontCamera ? 1 : -1);
									m_pDrive->TurnByAngle(dTheta);
								}
								bBallHunting = true;
								break;
							}
						}
						else if(iBallCount >= 2) {
							// Stop the drive
							m_pDrive->SetDriveSpeeds(0.000, 0.000);

							// We've grabbed atleast 2 balls, now lets try and shoot them...
							if(pObjDetection->m_kClass == DetectionClass::eHub) {
								// TODO: Drive odometry to determine the side we are on of the hub.
								const units::degree_t driveRotation = m_pDrive->m_pOdometry->GetPose().Rotation().Degrees();
								
								// We're in range
								if(CTrajectoryConstants::IsInShootingRange(pObjDetection->m_nDepth)) {

								}
								// Move closer to the hub
								else if(pObjDetection->m_nDepth < (CTrajectoryConstants::m_dAutoShootingDistance - CTrajectoryConstants::m_dAutoShootingRange)) {
									if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eFrontCamera) m_pDrive->SetDriveSpeeds(6.000, 6.000); 
									else if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eBackCamera) m_pDrive->SetDriveSpeeds(-6.000, -6.000);
								}
								// Move farther from the hub
								else if(pObjDetection->m_nDepth > (CTrajectoryConstants::m_dAutoShootingDistance + CTrajectoryConstants::m_dAutoShootingRange)) {
									if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eFrontCamera) m_pDrive->SetDriveSpeeds(-6.000, -6.000); 
									else if(pVisionPacket->m_kDetectionLocation == DetectionLocation::eBackCamera) m_pDrive->SetDriveSpeeds(6.000, 6.000);
								}
							}
						}
					}

					// We aren't on the hunt for a ball *AND* we don't have 2 balls in yet.
					// This means there's not a ball
					if(iBallCount < 2 && !bBallHunting) {
						// Spin 90deg in order to hopefully try and find a ball in either of the directions.
						m_pDrive->TurnByAngle(90);
					}
				}
			}
			*/
			break;
	}
}

/******************************************************************************
    Description:	Teleop initialization function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::TeleopInit()
{
	m_pDrive->Init();
	m_pDrive->SetJoystickControl(true);
	m_pDrive->SetDriveSafety(false);
	m_pBackIntake->Init();
	m_pLeftLift->Init();
	m_pRightLift->Init();
	m_pShooter->SetSafety(false);
	m_pShooter->Init();
	m_pShooter->StartFlywheelShot();
	m_pTransfer->Init();
}

/******************************************************************************
    Description:	Teleop 20ms periodic function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::TeleopPeriodic()
{
	/**************************************************************************
	    Description:	Drive stop and Joystick handling
	**************************************************************************/

	// If the drive controller is pressing Select, stop the drive train
	if (m_pDriveController->GetRawButtonPressed(eBack))
	{
		m_pDrive->ForceStop();
	}
	if (m_pDriveController->GetRawButtonReleased(eBack))
	{
		m_pDrive->SetJoystickControl(true);
	}
	m_pDrive->Tick();

	/**************************************************************************
	    Description:	Manual ticks
	**************************************************************************/

	// You could use a chunky if statement for this logic, but that'd be horrible, so, no (:
	int iBallCount = 0;
	for(int i = 0; i < 2; i++) {
		if(m_pTransfer->m_aBallLocations[i]) iBallCount += 1;
	}

	if(m_pDriveController->GetRawButtonPressed(eButtonRB)) {
		// If the intake isn't down, move the intake down
		if(!m_pBackIntake->GetLimitSwitchState(false)) {
			m_pBackIntake->MoveIntake(false);
			m_pBackIntake->m_bGoal = false;
		}
	}
	else if(m_pDriveController->GetRawButtonReleased(eButtonRB)) {
		// If the intake isn't up, move the intake up
		if(!m_pBackIntake->GetLimitSwitchState(true)) {
			m_pBackIntake->StopIntake();
			m_pBackIntake->MoveIntake(true);
			m_pBackIntake->m_bGoal = true;
		}
	}

	// When we've hit our goal, stop the motors (or start the intake in case of going down)
	if (m_pBackIntake->IsGoalPressed())
	{
		if (!m_pBackIntake->m_bGoal) m_pBackIntake->StartIntake();
		else m_pBackIntake->StopIntake();
		
		m_pBackIntake->StopDeploy();
		m_pBackIntake->m_bGoal = !m_pBackIntake->m_bGoal;
	}
	

	// We need to do some jankiness in order to lock the value of the vertical transfer sensor for shooting...	
	// If the front/back intake is on, and we don't have a ball in the vertical, start the front/back transfer
	if (m_pBackIntake->m_bIntakeOn && !m_pTransfer->m_aBallLocations[0]) {
		m_pTransfer->StartBack();
	}
	
	// If we have more than two balls at a time, we need to stop the intakes that we're running AND transfers to avoid shuffling the balls.
	if(iBallCount >= 2) {
		// We need to stop the horizontal transfers as well.
		m_pTransfer->StopBack();
	}

	// Send balls into the flywheel with the trigger (even if we don't technically have a ball in the vertical)
	if (m_pAuxController->GetRawAxis(eRightTrigger) >= 0.95) {
		m_pTransfer->StartVerticalShot();
	}
	// If a ball is in the vertical and RT isn't fully pressed, stop the vertical transfer
	else if(m_pTransfer->m_aBallLocations[0]) {
		m_pTransfer->StopVertical();
	}
	else if(!m_pTransfer->m_aBallLocations[0]) {
		m_pTransfer->StartVertical();
		// We need to feed the balls in the horizontal into the vertical and let them sit there.
		if(iBallCount >= 1) {
			m_pTransfer->StartBack();
		}
	}

	// Adjust the velocity of the flywheel with the DPad up/down.
	if (m_pAuxController->GetPOV() == 0) m_pShooter->AdjustVelocity(0.001);
	if (m_pAuxController->GetPOV() == 180) m_pShooter->AdjustVelocity(-0.001);

	// If both Lift mechanisms are ready, proceed to each stage based on button presses
	if (m_pLeftLift->m_bReady && m_pRightLift->m_bReady) {
		if (m_pAuxController->GetRawButton(eButtonRB) && m_pAuxController->GetRawButton(eButtonA) && m_kClimbState == eNoClimb) m_kClimbState = eMid;
		if (m_pAuxController->GetRawButton(eButtonRB) && m_pAuxController->GetRawButton(eButtonB) && m_kClimbState == eMid) m_kClimbState = eHigh;
		if (m_pAuxController->GetRawButton(eButtonRB) && m_pAuxController->GetRawButton(eButtonY) && m_kClimbState == eHigh) m_kClimbState = eTraverse;
		if (m_pAuxController->GetRawButton(eButtonRB) && m_pAuxController->GetRawButton(eButtonX) && m_kClimbState == eTraverse) m_kClimbState = eHang;
	}
	m_pLeftLift->Tick(m_kClimbState);
	m_pRightLift->Tick(m_kClimbState);

	/**************************************************************************
	    Description:	Vision processing and ball trackings
	**************************************************************************/
	
	// TODO: add vision functionality from TestPeriodic
}

/******************************************************************************
    Description:	Disabled initialization function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::DisabledInit()
{
	m_pDrive->SetJoystickControl(false);
	m_pDrive->SetDriveSafety(true);
}

/******************************************************************************
    Description:	Disabled 20ms periodic function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::DisabledPeriodic()
{

}

/******************************************************************************
    Description:	Test initialization function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::TestInit()
{
	m_pDrive->Init();
	m_pDrive->SetJoystickControl(true);
	m_pPrevVisionPacket = new CVisionPacket();
}

/******************************************************************************
    Description:	Test 20ms periodic function
	Arguments:		None
	Returns:		Nothing
******************************************************************************/
void CRobotMain::TestPeriodic()
{
	/**************************************************************************
	    Description:	Vision processing and ball trackings
	**************************************************************************/

	// Retrieve most recent vision packet
	CVisionPacket* pVisionPacket = CVisionPacket::GetReceivedPacket();
	// If it's not a nullptr, then continue, otherwise tick Drive
	if(pVisionPacket != nullptr && m_pDriveController->GetRawButton(eButtonA))
	{
		// If it's not the same as the last, continue, otherwise tick Drive
		if(m_pPrevVisionPacket->m_nRandVal != pVisionPacket->m_nRandVal)
		{
			// Parse detections and determine appropriate color alliance
			pVisionPacket->ParseDetections();
			DetectionClass kBallClass = (DriverStation::GetAlliance() == DriverStation::Alliance::kBlue ? eBlueCargo : eRedCargo);

			// Loop through the detections until the first correctly colored ball is found
			int* n = new int();
			for (*n = 0; (pVisionPacket->m_pDetections[*n]->m_kClass != kBallClass) && (*n < 15); *n += 1);

			// If n is a nullptr (i.e. wasn't looped through), tick drive instead of following ball
			if (n != nullptr)
			{
				CVisionPacket::sObjectDetection* pValidBall = pVisionPacket->m_pDetections[*n];

				// Find the angle from the center of the FoV and the amount of FoV it takes up
				const double dTheta = (pValidBall->m_nX - 160) * dAnglePerPixel;
				const double dHalfWidthAngle = (pValidBall->m_nWidth / 2) * dAnglePerPixel;

				// If dTheta is not within the angle taken up by the ball, then turn at 15% power
				// NOTE: TurnByPower(double) requests the value in percent form, not decimal
				if (fabs(dTheta) > dHalfWidthAngle) m_pDrive->TurnByPower(15.000);
			}
			else m_pDrive->Tick();
		}
		else m_pDrive->Tick();
	}
	else m_pDrive->Tick();
	
	m_pPrevVisionPacket = pVisionPacket;
}

#ifndef RUNNING_FRC_TESTS
int main() {
  return frc::StartRobot<CRobotMain>();
}
#endif