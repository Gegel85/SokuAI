//
// Created by PinkySmile on 08/12/2021.
//

#include <cassert>
#include <cmath>
#include <algorithm>
#include "GenNeuron.hpp"

namespace Trainer
{
	float GenNeuron::getValue()
	{
		if (!this->_computed) {
			this->_val = 0;
			this->_computed = true;
			for (auto &link : this->_links) {
				this->_val += link.neuron.getValue() * link.weight;
				assert(!std::isnan(this->_val));
			}
		}
		assert(!std::isnan(this->_val));
		return this->_val > 0 ? 1.f : 0.f;
	}

	void GenNeuron::startComputed()
	{
		this->_computed = false;
	}

	void GenNeuron::addLink(float weight, Neuron &neuron)
	{
		this->_links.emplace_back(neuron, weight);
	}
}