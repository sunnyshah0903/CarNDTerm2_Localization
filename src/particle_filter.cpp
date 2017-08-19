/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// TODO: Set the number of particles. Initialize all particles to first position (based on estimates of
	//   x, y, theta and their uncertainties from GPS) and all weights to 1.
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
	if(!is_initialized)
	{
		num_particles = 100;
		std::default_random_engine gen;
		std::normal_distribution<double> N_x(x,std[0]);
		std::normal_distribution<double> N_y(y,std[1]);
		std::normal_distribution<double> N_theta(theta,std[2]);
		particles.clear();
		weights.clear();
		for (int i=0 ; i< num_particles;i++)
		{
			Particle particle;
			particle.id = i;
			particle.x = N_x(gen);
			particle.y = N_y(gen);
			particle.theta = N_theta(gen);
			particle.weight = 1.0;		
	
			particles.push_back(particle);
			weights.push_back(1.0);
		}	

		is_initialized = true;
	}
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// TODO: Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/

	default_random_engine gen;

	for (int i=0 ; i< num_particles;i++)
	{
		double new_x;
		double new_y;
		double new_theta;

		if (yaw_rate == 0)
		{
			new_x = particles[i].x + velocity * delta_t * cos(particles[i].theta);
			new_y = particles[i].x + velocity * delta_t * sin(particles[i].theta);
			new_theta = particles[i].theta;
			//new_theta = particles[i].theta + yaw_rate*delta_t;

		}
		else
		{
			new_x = particles[i].x + velocity/yaw_rate * (sin(particles[i].theta + yaw_rate * delta_t) - sin(particles[i].theta));
			new_y = particles[i].y + velocity/yaw_rate * (cos(particles[i].theta) - cos(particles[i].theta + yaw_rate*delta_t));
			new_theta = particles[i].theta + yaw_rate*delta_t;
		}

		normal_distribution<double> N_x(new_x, std_pos[0]);
		normal_distribution<double> N_y(new_y, std_pos[1]);
		normal_distribution<double> N_theta(new_theta, std_pos[2]);

		particles[i].x = N_x(gen);
		particles[i].y = N_y(gen);
		particles[i].theta = N_theta(gen);
	}
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to
	//   implement this method and use it as a helper during the updateWeights phase.


	for (auto& obs : observations) {
		double minDist = std::numeric_limits<float>::max();
		for (const auto& pred : predicted) {
			double distance = dist(obs.x, obs.y, pred.x, pred.y);
			if (minDist > distance) {
				minDist = distance;
				obs.id = pred.id;
			}
		}
	}

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[],
		std::vector<LandmarkObs> observations, Map map_landmarks) {
	// TODO: Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html



	//std::cout << "updateWeights enter" << std::endl;
	double sig_x = std_landmark[0];
	double sig_y = std_landmark[1];

	for (int i =0 ; i< num_particles; i++)
	{
		//std::cout << "updateWeights for each weight" << std::endl;

		std::vector<LandmarkObs> trans_observations;
		std::vector<LandmarkObs> close_observations;
		trans_observations.clear();
		close_observations.clear();
		particles[i].weight = 1.0;

		// Step 1. Create a list of transformed observations
		for (int j = 0 ; j< observations.size() ; j++)
		{
			//std::cout << "creating transformed observations" << std::endl;
			LandmarkObs temp;
			temp.x = particles[i].x + (observations[j].x*cos(particles[i].theta) - observations[j].y*sin(particles[i].theta));
			temp.y = particles[i].y + (observations[j].x*sin(particles[i].theta) + observations[j].y*cos(particles[i].theta));
			temp.id = observations[j].id;
			trans_observations.push_back(temp);
		}

		// Step 2. Create a list of observations within sensor range
		for (int k = 0 ; k < map_landmarks.landmark_list.size() ; k++)
		{
			//std::cout << "identifying observations in range" << std::endl;
			Map::single_landmark_s landmark = map_landmarks.landmark_list[k];

			double distance = dist(particles[i].x,particles[i].y, landmark.x_f,landmark.y_f);
			if(distance < sensor_range)
			{
				//std::cout << "got it !" << std::endl;
				LandmarkObs temp;
				temp.x = landmark.x_f;
				temp.y = landmark.y_f;
				temp.id = landmark.id_i;
				close_observations.push_back(temp);

			}

		}

		// Step 3. data association
		//std::cout << "data association" << std::endl;
		double n_weight =1.0;
		//vector<int> P_associations;
		//vector<double> P_sense_x;
    	//vector<double> P_sense_y;
    	//P_associations.clear();
    	//P_sense_x.clear();
    	//P_sense_y.clear();
		for (int j = 0 ; j< trans_observations.size() ; j++)
		{
			//std::cout << "looping through observations" << std::endl;
			double min_distance = 99999.0;
			LandmarkObs temp;
			for (int k = 0 ; k < close_observations.size() ; k++)
			{
				//std::cout << "and close landmarks" << std::endl;
				double distance = dist(trans_observations[j].x,trans_observations[j].y,close_observations[k].x,close_observations[k].y);
				if (distance < min_distance)
				{
					//std::cout << "found closest" << std::endl;
					min_distance = distance;
					temp.x = close_observations[k].x;
					temp.y = close_observations[k].y;
					temp.id = close_observations[k].id;
					trans_observations[j].id = close_observations[k].id;
				}
			}

			double mu_x = temp.x - trans_observations[j].x;
			double mu_y = temp.y - trans_observations[j].y;
			//std::cout << "complex weight calculation" << std::endl;
//			double mu_x = trans_observations[j].x - temp.x;
//			double mu_y = trans_observations[j].y - temp.y;

			double gauss_norm = 1 / (2*3.1415*sig_x*sig_y);

			double eydeno = 2*pow(sig_y,2);
			double exdeno = 2*pow(sig_x,2);
			double exnum = pow(mu_x,2);
			double eynum = pow(mu_y,2);
			double exponent =  std::exp(-((exnum/exdeno) - (eynum/eydeno)));
			double temp_weight = gauss_norm * exponent;
			std::cout << "printing variables   " << temp_weight << exponent << "   " << exnum << "   " << eynum << "   " << exdeno << "   " << eydeno << std::endl;
			if(temp_weight!=0)
				n_weight *= temp_weight;

        //    P_associations.push_back(trans_observations[j].id);
        //    P_sense_x.push_back(trans_observations[j].x);
        //    P_sense_y.push_back(trans_observations[j].y);
		}

		//std::cout << "assigning weight   " << n_weight  << std::endl;
		particles[i].weight = n_weight;
		weights[i] = n_weight;
		//particles[i] = SetAssociations(particles[i], P_associations, P_sense_x, P_sense_y);

		//std::cout << "done for the particle" << std::endl;
	}
	std::cout << "updateWeights exit" << std::endl;


}

void ParticleFilter::resample() {
	// TODO: Resample particles with replacement with probability proportional to their weight.
	// NOTE: You may find std::discrete_distribution helpful here.
	//   http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

	default_random_engine gen;
	discrete_distribution <int> distribution (weights.begin(), weights.end());

	vector<Particle> resample_particles;

	for (int i=0; i< num_particles;i++)
	{
		resample_particles.push_back(particles[distribution(gen)]);
	}

	particles = resample_particles;
}

Particle ParticleFilter::SetAssociations(Particle particle, std::vector<int> associations, std::vector<double> sense_x, std::vector<double> sense_y)
{
	//particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
	// associations: The landmark id that goes along with each listed association
	// sense_x: the associations x mapping already converted to world coordinates
	// sense_y: the associations y mapping already converted to world coordinates

	//Clear the previous associations
	particle.associations.clear();
	particle.sense_x.clear();
	particle.sense_y.clear();

	particle.associations= associations;
 	particle.sense_x = sense_x;
 	particle.sense_y = sense_y;

 	return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}

