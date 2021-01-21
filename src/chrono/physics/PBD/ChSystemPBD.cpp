// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Simone Benatti
// =============================================================================
//
// Physical system in which contact is modeled using a non-smooth
// (complementarity-based) method.
//
// =============================================================================

#include <algorithm>

#include "chrono/physics/PBD/ChSystemPBD.h"
#include "chrono/physics/PBD/ChContactContainerPBD.h"
#include "chrono/physics/ChContactContainerNSC.h"
#include "chrono/physics/ChProximityContainer.h"
#include "chrono/physics/ChSystem.h"
#include "chrono/collision/ChCollisionSystemBullet.h"

namespace chrono {

	// Register into the object factory, to enable run-time dynamic creation and persistence
	CH_FACTORY_REGISTER(ChSystemPBD)

		ChSystemPBD::ChSystemPBD(bool init_sys)
		: ChSystem() {
		if (init_sys) {
			// Set default contact container
			contact_container = chrono_types::make_shared<ChContactContainerPBD>();
			contact_container->SetSystem(this);

			// Set default collision engine
			collision_system = chrono_types::make_shared<collision::ChCollisionSystemBullet>();

			// Set the system descriptor
			descriptor = chrono_types::make_shared<ChSystemDescriptor>();

			// Set default solver
			// Not called by Integrate_Y. Users might call it through DoAssembly and such.
			SetSolverType(ChSolver::Type::PSOR);
		}

		// Set default collision envelope and margin.
		collision::ChCollisionModel::SetDefaultSuggestedEnvelope(0.03);
		collision::ChCollisionModel::SetDefaultSuggestedMargin(0.01);
	}

	ChSystemPBD::ChSystemPBD(const ChSystemPBD& other) : ChSystem(other) {}

	void ChSystemPBD::SetContactContainer(std::shared_ptr<ChContactContainer> container) {
		if (std::dynamic_pointer_cast<ChContactContainerPBD>(container))
			ChSystem::SetContactContainer(container);
	}

	// -----------------------------------------------------------------------------
	//  PERFORM INTEGRATION STEP  using pluggable timestepper
	// -----------------------------------------------------------------------------


	bool ChSystemPBD::Integrate_Y() {
		CH_PROFILE("Integrate_Y");

		ResetTimers();

		timer_step.start();

		stepcount++;
		solvecount = 0;
		setupcount = 0;

		// Compute contacts and create contact constraints
		int ncontacts_old = ncontacts;
		ComputeCollisions();

		// Declare an NSC system as "out of date" if there are contacts
		if (GetContactMethod() == ChContactMethod::NSC && (ncontacts_old != 0 || ncontacts != 0))
			is_updated = false;

		// Counts dofs, number of constraints, statistics, etc.
		// Note: this must be invoked at all times (regardless of the flag is_updated), as various physics items may use
		// their own Setup to perform operations at the beginning of a step.
		Setup();

		// If needed, update everything. No need to update visualization assets here.
		if (!is_updated) {
			Update(false);
		}


		// Re-wake the bodies that cannot sleep because they are in contact with
		// some body that is not in sleep state.
		//TODO: I had to change ChSystem::ManageSleepingBodies from private to protected. 
		// Not sure if this is useful, maybe skip smth is abody is sleeping?
		ManageSleepingBodies();
		// Prepare lists of variables and constraints.
		// TODO: check if this is needed by  PBD (constrint matrix is not, but maybe we process the state here)
		DescriptorPrepareInject(*descriptor);


		// If needed, update everything. No need to update visualization assets here.
		if (!PBD_isSetup) {
			PBDSetup();
			PBD_isSetup = true;
		}

		// PERFORM TIME STEP HERE!
		{
			CH_PROFILE("Advance");
			timer_advance.start();
			this->Advance();
			timer_advance.stop();
		}

		// Executes custom processing at the end of step
		CustomEndOfStep();

		// Time elapsed for step
		timer_step.stop();

		// Tentatively mark system as unchanged (i.e., no updated necessary)
		is_updated = true;

		return true;
	}


	void ChSystemPBD::ArchiveOUT(ChArchiveOut& marchive) {
		// version number
		marchive.VersionWrite<ChSystemPBD>();

		// serialize parent class
		ChSystem::ArchiveOUT(marchive);

		// serialize all member data:
	}

	// Method to allow de serialization of transient data from archives.
	void ChSystemPBD::ArchiveIN(ChArchiveIn& marchive) {
		// version number
		int version = marchive.VersionRead<ChSystemPBD>();

		// deserialize parent class
		ChSystem::ArchiveIN(marchive);

		// stream in all member data:
	}

	void ChSystemPBD::PBDSetup() {
		// create the list of links using PBD formulation
		for (auto& value : Get_linklist()) {
			if (dynamic_cast<const ChLinkLock*>(value.get()) != nullptr) {
				ChLinkLock* linkll = dynamic_cast< ChLinkLock*>(value.get());
				auto pbdlink = chrono_types::make_shared<ChLinkPBDLock>(linkll, this);
				linklistPBD.push_back(pbdlink);
			}
			else if (dynamic_cast<const ChLinkMotor*>(value.get()) != nullptr) {
				ChLinkMotor* linkmg = dynamic_cast<ChLinkMotor*>(value.get());
				auto pbdlink = chrono_types::make_shared<ChLinkPBDMotor>(linkmg, this);
				linklistPBD.push_back(pbdlink);
			}
			else if (dynamic_cast<const ChLinkMateGeneric*>(value.get()) != nullptr) {
				ChLinkMateGeneric* linkmg = dynamic_cast<ChLinkMateGeneric*>(value.get());
				auto pbdlink = chrono_types::make_shared<ChLinkPBDMate>(linkmg, this);
				linklistPBD.push_back(pbdlink);
			}
			else if (dynamic_cast<const ChLinkUniversal*>(value.get()) != nullptr) {
				ChLinkUniversal* linkmg = dynamic_cast<ChLinkUniversal*>(value.get());
				auto pbdlink = chrono_types::make_shared<ChLinkPBDUniversal>(linkmg, this);
				linklistPBD.push_back(pbdlink);
			}
			else if (dynamic_cast<const ChLinkDistance*>(value.get()) != nullptr) {
				ChLinkDistance* linkmg = dynamic_cast<ChLinkDistance*>(value.get());
				auto pbdlink = chrono_types::make_shared<ChLinkPBDDistance>(linkmg, this);
				linklistPBD.push_back(pbdlink);
			}
			else if (dynamic_cast<const ChLinkTSDA*>(value.get()) != nullptr || dynamic_cast<const ChLinkRotSpringCB*>(value.get()) != nullptr) {
				continue;
			}
			else
			{
				std::cout << "Link not managed by PBD implementation\n";
				throw std::invalid_argument("One or more of the system links cannot be treated as PBD link");
			}

		}
		// The gyroscopic term is evaluated within the PBD loop
		for (auto& body : Get_bodylist()) {
			body->SetNoGyroTorque(true);
		}
		// allocate the proper amount of vectors and quaternions
		int n = Get_bodylist().size();
		x_prev.resize(n);
		q_prev.resize(n);

	}

	void ChSystemPBD::SolvePositions() {
		for (auto& link : linklistPBD) {
			link->SolvePositions();
		}
	}

	void ChSystemPBD::SolveContacts(double h) {
		int n = contactlistPBD.size();
//#pragma omp parallel for 
		for (int i = 0; i < n; i++) {
			contactlistPBD[i]->SolveContactPositions();
		}
	}

	void ChSystemPBD::SolveVelocities(double h) {
		int n = contactlistPBD.size();
//#pragma omp parallel for 
		for (int i = 0; i < n; i++) {
			contactlistPBD[i]->SolveVelocity();
		}
	}

	void ChSystemPBD::CollectContacts() {
		// TODO: can we do something less brutal and use the previous knowledge?
		contactlistPBD.clear();
		ChContactContainerPBD* cc;
		// The cc of a PBD system is always a PBDcc
		cc = static_cast<ChContactContainerPBD*>(this->contact_container.get());
		auto cl = cc->Get_6_6_clist();
		for (auto& contact : cl) {
			// !!! BEWARE !!! Assuming contactable to be bodies. 
			ChBody* body1 = static_cast<ChBody*>(contact->GetObjA());
			ChBody* body2 = static_cast<ChBody*>(contact->GetObjB());
			double frict = contact->GetFriction();
			// contact points in rel coors
			ChVector<> p1 = body1->GetRot().RotateBack(contact->GetContactP1() - body1->GetPos());
			ChVector<> p2 = body2->GetRot().RotateBack(contact->GetContactP2() - body2->GetPos());
			ChVector<> norm = contact->GetContactNormal();
			// orientation of the contact frame wrt the body1 frame. the contact frame has the x axis aligned with the normal, so:
			ChQuaternion<> q_cb1 = Q_from_Vect_to_Vect(body1->GetRot().Rotate(VECT_X), norm);
			ChQuaternion<> q_cb2 = Q_from_Vect_to_Vect(body2->GetRot().Rotate(VECT_X), norm);
			ChFrame<double> frame1(p1, q_cb1);
			ChFrame<double> frame2(p2, q_cb2);
			auto contact = std::make_shared<ChContactPBD>(body1, body2, this, frame1, frame2, frict);
			contactlistPBD.push_back(contact);
		}
	}

	// Implementation af algorithm 2 from Detailed Rigid Body Simulation with Extended Position Based Dynamics, Mueller et al.
	void ChSystemPBD::Advance() {
		// Update the contact pairs
		CollectContacts();
		int n = Get_bodylist().size();
		h = step / substeps;
		for (int i = 0; i < substeps; i++) {
			// Used to contraint static friction
			// delete this when possible and use a more efficient way to save/update state
			//SaveOldPos();
			//#pragma omp parallel for 
			for (int j = 0; j < n; j++) {
				std::shared_ptr<ChBody> body = Get_bodylist()[j];
				if (body->GetBodyFixed() == true) {
					continue;
				}
				x_prev[j] = body->GetPos();
				q_prev[j] = body->GetRot();
				ChVector<> v = body->GetPos_dt() + h * (1 / body->GetMass()) * body->GetAppliedForce();
				body->SetPos_dt(v);
				body->SetPos(x_prev[j] + h * v);
				// gyroscopic effects also in body->ComputeGyro
				ChVector<> T = body->GetAppliedTorque();
				ChVector<> G = body->GetWvel_loc().Cross(body->GetInertia() *  body->GetWvel_loc());
				ChVector<> omega = body->GetWvel_loc() + body->GetInvInertia() * (T - G)*h;
				body->SetWvel_loc(omega);
				// !!! different multiplication order within Qdt_from_Wrel w.r.t. the paper
				// due to local vs global omegas !!!
				ChQuaternion<> dqdt;
				// dq/dt =  1/2 {q}*{0,w_rel}
				dqdt.Qdt_from_Wrel(omega, q_prev[j]);
				// q1 = q0 + dq/dt * h
				ChQuaternion<> qnew = q_prev[j] + dqdt *h;
				qnew.Normalize();
				body->SetRot(qnew);
			}
			// Correct positions to respect constraints. "numPosIters"set to 1 according to the paper
			SolvePositions();
			SolveContacts(h);

			// Update velocities to take corrected positions into account
			//#pragma omp parallel for 
			for (int j = 0; j < n; j++) {
				std::shared_ptr<ChBody> body = Get_bodylist()[j];
				if (body->GetBodyFixed()) {
					continue;
				}
				ChVector<> new_vel = (body->GetPos() - x_prev[j]) / h;
				body->SetPos_dt(new_vel);
				ChQuaternion<> deltaq = body->GetRot() * q_prev[j].GetInverse();
				deltaq *= 2 / h;
				ChVector<> new_om = (deltaq.e0() > 0) ? deltaq.GetVector() : -deltaq.GetVector();
				body->SetWvel_par(new_om);

				//body->coord_dt.rot = deltaq;
			}
			// Scatter updated state
			// Similarly we contraint normal (and, if static, tangential) displacement in contacts
			SolveVelocities(h);

			T += h;

		}
		SetChTime(T);
		// Scatter Gather influences only collision detaction -> do after substepping
		StateSetup(sys_state, sys_state_delta, sys_acc);
		StateGather(sys_state, sys_state_delta, T);
		StateScatter(sys_state, sys_state_delta, T, true);
	}
}  // end namespace chrono
