//
// Created by PinkySmile on 06/12/2021.
//

#include <fstream>
#include <random>
#include "GeneticAI.hpp"
#include "GenNeuron.hpp"

#undef max
#undef min

#define STATE_NEURONS_COUNT (38 + 3 + 14)
#define INPUT_NEURONS_COUNT (STATE_NEURONS_COUNT * 2 + 3 + 2 + 4)
#define OBJECTS_OFFSET 0
#define WEATHER_OFFSET 3
#define STATE_OFFSET 5
#define HAND_INDEX 22
#define SKILLS_INDEX 24
#define NEW_NEURON_CHANCE_NUM 1
#define NEW_NEURON_CHANCE_DEN 10

#define DIVISOR_DIRECTION                   1.f
#define DIVISOR_OPPONENT_RELATIVE_POS_X     1240.f
#define DIVISOR_OPPONENT_RELATIVE_POS_Y     1240.f
#define DIVISOR_DIST_TO_LEFT_CORNER         1240.f
#define DIVISOR_DIST_TO_RIGHT_CORNER        1240.f
#define DIVISOR_DIST_TO_GROUND              1240.f
#define DIVISOR_ACTION                      1000.f
#define DIVISOR_ACTION_BLOCK_ID             50.f
#define DIVISOR_ANIMATION_COUNTER           50.f
#define DIVISOR_ANIMATION_SUBFRAME          50.f
#define DIVISOR_FRAME_COUNT                 65535.f
#define DIVISOR_COMBO_DAMAGE                10000.f
#define DIVISOR_COMBO_LIMIT                 200.f
#define DIVISOR_AIRBORNE                    1.f
#define DIVISOR_HP                          10000.f
#define DIVISOR_AIR_DASH_COUNT              3.f
#define DIVISOR_SPIRIT                      10000.f
#define DIVISOR_MAX_SPIRIT                  10000.f
#define DIVISOR_UNTECH                      200.f
#define DIVISOR_HEALING_CHARM               250.f
#define DIVISOR_SWORD_OF_RAPTURE            1200.f
#define DIVISOR_SCORE                       2.f
#define DIVISOR_HAND                        220.f
#define DIVISOR_CARD_GAUGE                  500.f
#define DIVISOR_SKILLS                      4.f
#define DIVISOR_FAN_LEVEL                   4.f
#define DIVISOR_DROP_INVUL_TIME_LEFT        1200.f
#define DIVISOR_SUPER_ARMOR_TIME_LEFT       1500.f
#define DIVISOR_SUPER_ARMOR_HP              3000.f
#define DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT 600.f
#define DIVISOR_PHILOSOFER_STONE_TIME_LEFT  1200.f
#define DIVISOR_SAKUYAS_WORLD_TIME_LEFT     300.f
#define DIVISOR_PRIVATE_SQUARE_TIME_LEFT    300.f
#define DIVISOR_ORRERIES_TIME_LEFT          600.f
#define DIVISOR_MPP_TIME_LEFT               480.f
#define DIVISOR_KANAKO_COOLDOWN             1500.f
#define DIVISOR_SUWAKO_COOLDOWN             1500.f
#define DIVISOR_OBJECT_COUNT                70.f

extern std::mt19937 random;

namespace Trainer
{
	GeneticAI::GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, const GeneticAI &parent1, const GeneticAI &parent2) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Creating child AI from %s and %s\n", parent1.toString().c_str(), parent2.toString().c_str());
		assert(parent1._genome.size() == parent2._genome.size());
		puts("Generating genome...");
		for (int i = 0; i < parent1._genome.size(); i++)
			this->_genome.push_back((random() % 2 ? parent1 : parent2)._genome[i]);
		if (random() % 1000 == 0)
			this->_genome[random() % this->_genome.size()].data[random() % 3] ^= (1 << (random() % 32));
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	GeneticAI::GeneticAI(unsigned generation, unsigned id, unsigned middleLayerSize, unsigned int genCount) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Creating new AI with %i neurons and %i genes\n", middleLayerSize, genCount);
		printf("Generating genome (bounds 0x%X-0x%X (%u))...\n", random.min(), random.max(), sizeof(Gene));
		for (int i = 0; i < genCount; i++) {
			this->_genome.push_back({});
			for (auto &data : this->_genome.back().data)
				data = random();
		}
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	GeneticAI::GeneticAI(unsigned int generation, unsigned int id, unsigned int middleLayerSize, unsigned int genCount, const std::string &path) :
		BaseAI(SokuLib::CHARACTER_REMILIA, 0),
		_generation(generation),
		_id(id)
	{
		printf("Loading AI from %s\n", path.c_str());

		std::ifstream stream{path};

		if (stream.fail())
			throw std::invalid_argument(path + ": " + strerror(errno));
		puts("Loading...");
		this->_genome.resize(genCount);
		stream.read(reinterpret_cast<char *>(this->_genome.data()), this->_genome.size() * sizeof(*this->_genome.data()));
		puts("Creating neurons...");
		this->_createNeurons(middleLayerSize);
		puts("Generating links...");
		this->_generateLinks();
		puts("Making palette...");
		this->_palette = this->_calculatePalette();
		puts("Done!");
	}

	void GeneticAI::save(const std::string &path) const
	{
		std::ofstream stream{path};

		if (stream.fail())
			throw std::invalid_argument(path + ": " + strerror(errno));
		stream.write(reinterpret_cast<const char *>(this->_genome.data()), this->_genome.size() * sizeof(*this->_genome.data()));
	}

	std::string GeneticAI::toString() const
	{
		return "GeneticAI gen" + std::to_string(this->_generation) + "-" + std::to_string(this->_id);
	}

	void GeneticAI::onWin(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onWin(myScore, opponentScore);
	}

	void GeneticAI::onLose(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onLose(myScore, opponentScore);
	}

	void GeneticAI::onTimeout(unsigned char myScore, unsigned char opponentScore)
	{
		BaseAI::onTimeout(myScore, opponentScore);
	}

	void GeneticAI::onGameStart(SokuLib::Character myChr, SokuLib::Character opponentChr, unsigned int inputDelay)
	{
		BaseAI::onGameStart(myChr, opponentChr, inputDelay);
	}

	GameInstance::PlayerParams GeneticAI::getParams() const
	{
		GameInstance::PlayerParams params{
			this->_character,
			this->_palette
		};

		buildDeck(this->_character, params.deck);
		strcpy_s(params.name, this->toString().c_str());
		return params;
	}

	Input GeneticAI::getInputs(const GameInstance::GameFrame &frame, bool isLeft)
	{
		this->_myObjects->objects = isLeft ? frame.leftObjects : frame.rightObjects;
		this->_opObjects->objects = isLeft ? frame.rightObjects : frame.leftObjects;

		// Weather
		this->_inNeurons[WEATHER_OFFSET + 0]->setValue(frame.displayedWeather <= 19 ? frame.displayedWeather / 19.f : -1);
		this->_inNeurons[WEATHER_OFFSET + 1]->setValue(frame.activeWeather <= 19 ? frame.activeWeather / 19.f : -1);
		this->_inNeurons[WEATHER_OFFSET + 2]->setValue(frame.weatherTimer / 999.f);

		// God, please forgive me for I have sinned
		this->_inNeurons[STATE_OFFSET + 0x00 + 0x00]->setValue((isLeft ? frame.left : frame.right).direction                / DIVISOR_DIRECTION);
		this->_inNeurons[STATE_OFFSET + 0x01 + 0x00]->setValue((isLeft ? frame.left : frame.right).opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_inNeurons[STATE_OFFSET + 0x02 + 0x00]->setValue((isLeft ? frame.left : frame.right).opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_inNeurons[STATE_OFFSET + 0x03 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x04 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x05 + 0x00]->setValue((isLeft ? frame.left : frame.right).distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_inNeurons[STATE_OFFSET + 0x06 + 0x00]->setValue((isLeft ? frame.left : frame.right).action                   / DIVISOR_ACTION);
		this->_inNeurons[STATE_OFFSET + 0x07 + 0x00]->setValue((isLeft ? frame.left : frame.right).actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_inNeurons[STATE_OFFSET + 0x08 + 0x00]->setValue((isLeft ? frame.left : frame.right).animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_inNeurons[STATE_OFFSET + 0x09 + 0x00]->setValue((isLeft ? frame.left : frame.right).animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_inNeurons[STATE_OFFSET + 0x0A + 0x00]->setValue((isLeft ? frame.left : frame.right).frameCount               / DIVISOR_FRAME_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x0B + 0x00]->setValue((isLeft ? frame.left : frame.right).comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_inNeurons[STATE_OFFSET + 0x0C + 0x00]->setValue((isLeft ? frame.left : frame.right).comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_inNeurons[STATE_OFFSET + 0x0D + 0x00]->setValue((isLeft ? frame.left : frame.right).airBorne                 / DIVISOR_AIRBORNE);
		this->_inNeurons[STATE_OFFSET + 0x0E + 0x00]->setValue((isLeft ? frame.left : frame.right).hp                       / DIVISOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x0F + 0x00]->setValue((isLeft ? frame.left : frame.right).airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x10 + 0x00]->setValue((isLeft ? frame.left : frame.right).spirit                   / DIVISOR_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x11 + 0x00]->setValue((isLeft ? frame.left : frame.right).maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x12 + 0x00]->setValue((isLeft ? frame.left : frame.right).untech                   / DIVISOR_UNTECH);
		this->_inNeurons[STATE_OFFSET + 0x13 + 0x00]->setValue((isLeft ? frame.left : frame.right).healingCharm             / DIVISOR_HEALING_CHARM);
		this->_inNeurons[STATE_OFFSET + 0x14 + 0x00]->setValue((isLeft ? frame.left : frame.right).swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_inNeurons[STATE_OFFSET + 0x15 + 0x00]->setValue((isLeft ? frame.left : frame.right).score                    / DIVISOR_SCORE);
		for (int i = 0; i < 5; i++) {
			auto card = (isLeft ? frame.left : frame.right).hand[i];

			this->_inNeurons[STATE_OFFSET + 0x16 + 0x00 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_inNeurons[STATE_OFFSET + 0x1B + 0x00]->setValue((isLeft ? frame.left : frame.right).cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = (isLeft ? frame.left : frame.right).skills[i];

			this->_inNeurons[STATE_OFFSET + 0x1C + 0x00 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_inNeurons[STATE_OFFSET + 0x2B + 0x00]->setValue((isLeft ? frame.left : frame.right).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_inNeurons[STATE_OFFSET + 0x2C + 0x00]->setValue((isLeft ? frame.left : frame.right).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		auto t = (isLeft ? frame.left : frame.right).superArmorTimeLeft;
		auto h = (isLeft ? frame.left : frame.right).superArmorHp;

		this->_inNeurons[STATE_OFFSET + 0x2D + 0x00]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x2E + 0x00]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x2F + 0x00]->setValue((isLeft ? frame.left : frame.right).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x30 + 0x00]->setValue((isLeft ? frame.left : frame.right).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x31 + 0x00]->setValue((isLeft ? frame.left : frame.right).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x32 + 0x00]->setValue((isLeft ? frame.left : frame.right).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x33 + 0x00]->setValue((isLeft ? frame.left : frame.right).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x34 + 0x00]->setValue((isLeft ? frame.left : frame.right).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x35 + 0x00]->setValue((isLeft ? frame.left : frame.right).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_inNeurons[STATE_OFFSET + 0x36 + 0x00]->setValue((isLeft ? frame.left : frame.right).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);



		this->_inNeurons[STATE_OFFSET + 0x00 + 0x37]->setValue((!isLeft ? frame.left : frame.right).direction                / DIVISOR_DIRECTION);
		this->_inNeurons[STATE_OFFSET + 0x01 + 0x37]->setValue((!isLeft ? frame.left : frame.right).opponentRelativePos.x    / DIVISOR_OPPONENT_RELATIVE_POS_X);
		this->_inNeurons[STATE_OFFSET + 0x02 + 0x37]->setValue((!isLeft ? frame.left : frame.right).opponentRelativePos.y    / DIVISOR_OPPONENT_RELATIVE_POS_Y);
		this->_inNeurons[STATE_OFFSET + 0x03 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToBackCorner         / DIVISOR_DIST_TO_LEFT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x04 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToFrontCorner        / DIVISOR_DIST_TO_RIGHT_CORNER);
		this->_inNeurons[STATE_OFFSET + 0x05 + 0x37]->setValue((!isLeft ? frame.left : frame.right).distToGround             / DIVISOR_DIST_TO_GROUND);
		this->_inNeurons[STATE_OFFSET + 0x06 + 0x37]->setValue((!isLeft ? frame.left : frame.right).action                   / DIVISOR_ACTION);
		this->_inNeurons[STATE_OFFSET + 0x07 + 0x37]->setValue((!isLeft ? frame.left : frame.right).actionBlockId            / DIVISOR_ACTION_BLOCK_ID);
		this->_inNeurons[STATE_OFFSET + 0x08 + 0x37]->setValue((!isLeft ? frame.left : frame.right).animationCounter         / DIVISOR_ANIMATION_COUNTER);
		this->_inNeurons[STATE_OFFSET + 0x09 + 0x37]->setValue((!isLeft ? frame.left : frame.right).animationSubFrame        / DIVISOR_ANIMATION_SUBFRAME);
		this->_inNeurons[STATE_OFFSET + 0x0A + 0x37]->setValue((!isLeft ? frame.left : frame.right).frameCount               / DIVISOR_FRAME_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x0B + 0x37]->setValue((!isLeft ? frame.left : frame.right).comboDamage              / DIVISOR_COMBO_DAMAGE);
		this->_inNeurons[STATE_OFFSET + 0x0C + 0x37]->setValue((!isLeft ? frame.left : frame.right).comboLimit               / DIVISOR_COMBO_LIMIT);
		this->_inNeurons[STATE_OFFSET + 0x0D + 0x37]->setValue((!isLeft ? frame.left : frame.right).airBorne                 / DIVISOR_AIRBORNE);
		this->_inNeurons[STATE_OFFSET + 0x0E + 0x37]->setValue((!isLeft ? frame.left : frame.right).hp                       / DIVISOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x0F + 0x37]->setValue((!isLeft ? frame.left : frame.right).airDashCount             / DIVISOR_AIR_DASH_COUNT);
		this->_inNeurons[STATE_OFFSET + 0x10 + 0x37]->setValue((!isLeft ? frame.left : frame.right).spirit                   / DIVISOR_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x11 + 0x37]->setValue((!isLeft ? frame.left : frame.right).maxSpirit                / DIVISOR_MAX_SPIRIT);
		this->_inNeurons[STATE_OFFSET + 0x12 + 0x37]->setValue((!isLeft ? frame.left : frame.right).untech                   / DIVISOR_UNTECH);
		this->_inNeurons[STATE_OFFSET + 0x13 + 0x37]->setValue((!isLeft ? frame.left : frame.right).healingCharm             / DIVISOR_HEALING_CHARM);
		this->_inNeurons[STATE_OFFSET + 0x14 + 0x37]->setValue((!isLeft ? frame.left : frame.right).swordOfRapture           / DIVISOR_SWORD_OF_RAPTURE);
		this->_inNeurons[STATE_OFFSET + 0x15 + 0x37]->setValue((!isLeft ? frame.left : frame.right).score                    / DIVISOR_SCORE);
		for (int i = 0; i < 5; i++) {
			auto card = (!isLeft ? frame.left : frame.right).hand[i];

			this->_inNeurons[STATE_OFFSET + 0x16 + 0x37 + i]->setValue(card == 0xFF ? -1 : (card / DIVISOR_HAND));
		}
		this->_inNeurons[STATE_OFFSET + 0x1B + 0x00]->setValue((isLeft ? frame.left : frame.right).cardGauge                / DIVISOR_CARD_GAUGE);
		for (int i = 0; i < 15; i++) {
			auto skill = (!isLeft ? frame.left : frame.right).skills[i];

			this->_inNeurons[STATE_OFFSET + 0x1C + 0x37 + i]->setValue(skill.notUsed ? -1 : (skill.level / DIVISOR_SKILLS));
		}
		this->_inNeurons[STATE_OFFSET + 0x2B + 0x37]->setValue((!isLeft ? frame.left : frame.right).fanLevel                 / DIVISOR_FAN_LEVEL);
		this->_inNeurons[STATE_OFFSET + 0x2C + 0x37]->setValue((!isLeft ? frame.left : frame.right).dropInvulTimeLeft        / DIVISOR_DROP_INVUL_TIME_LEFT);

		t = (!isLeft ? frame.left : frame.right).superArmorTimeLeft;
		h = (!isLeft ? frame.left : frame.right).superArmorHp;

		this->_inNeurons[STATE_OFFSET + 0x2D + 0x37]->setValue(t < 0 ? t : t / DIVISOR_SUPER_ARMOR_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x2E + 0x37]->setValue(h < 0 ? h : h / DIVISOR_SUPER_ARMOR_HP);
		this->_inNeurons[STATE_OFFSET + 0x2F + 0x37]->setValue((!isLeft ? frame.left : frame.right).milleniumVampireTimeLeft / DIVISOR_MILLENIUM_VAMPIRE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x30 + 0x37]->setValue((!isLeft ? frame.left : frame.right).philosoferStoneTimeLeft  / DIVISOR_PHILOSOFER_STONE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x31 + 0x37]->setValue((!isLeft ? frame.left : frame.right).sakuyasWorldTimeLeft     / DIVISOR_SAKUYAS_WORLD_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x32 + 0x37]->setValue((!isLeft ? frame.left : frame.right).privateSquareTimeLeft    / DIVISOR_PRIVATE_SQUARE_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x33 + 0x37]->setValue((!isLeft ? frame.left : frame.right).orreriesTimeLeft         / DIVISOR_ORRERIES_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x34 + 0x37]->setValue((!isLeft ? frame.left : frame.right).mppTimeLeft              / DIVISOR_MPP_TIME_LEFT);
		this->_inNeurons[STATE_OFFSET + 0x35 + 0x37]->setValue((!isLeft ? frame.left : frame.right).kanakoCooldown           / DIVISOR_KANAKO_COOLDOWN);
		this->_inNeurons[STATE_OFFSET + 0x36 + 0x37]->setValue((!isLeft ? frame.left : frame.right).suwakoCooldown           / DIVISOR_SUWAKO_COOLDOWN);
		this->_inNeurons[STATE_OFFSET + 0x6E]->setValue((random() - random.min()) * 2 / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x6F]->setValue((random() - random.min()) * 2 / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x70]->setValue((random() - random.min()) * 2 / (random.max() - random.min()) - 1);
		this->_inNeurons[STATE_OFFSET + 0x71]->setValue((random() - random.min()) * 2 / (random.max() - random.min()) - 1);

		for (auto &neuron : this->_neurons)
			reinterpret_cast<GenNeuron &>(*neuron).startComputed();
		for (auto &neuron : this->_outNeurons)
			reinterpret_cast<GenNeuron &>(*neuron).startComputed();

		try {
			return {
				this->_outNeurons[0]->getValue() >= 0.5,
				this->_outNeurons[1]->getValue() >= 0.5,
				this->_outNeurons[2]->getValue() >= 0.5,
				this->_outNeurons[3]->getValue() >= 0.5,
				this->_outNeurons[4]->getValue() >= 0.5,
				this->_outNeurons[5]->getValue() >= 0.5,
				static_cast<char>((this->_outNeurons[6]->getValue() >= 0.5) - (this->_outNeurons[6]->getValue() <= -0.5)),
				static_cast<char>((this->_outNeurons[7]->getValue() >= 0.5) - (this->_outNeurons[7]->getValue() <= -0.5)),
			};
		} catch (std::exception &e) {
			puts(e.what());
			throw;
		}
	}

	unsigned int GeneticAI::getId() const
	{
		return this->_id;
	}

	int GeneticAI::getGeneration() const
	{
		return this->_generation;
	}

	void GeneticAI::_createNeurons(unsigned middleLayerSize)
	{
		this->_inNeurons.clear();
		this->_inNeurons.reserve(STATE_NEURONS_COUNT);
		this->_myObjects = new ObjectsNeuron(0);
		this->_opObjects = new ObjectsNeuron(1);
		this->_inNeurons.emplace_back(this->_myObjects);
		this->_inNeurons.emplace_back(this->_opObjects);
		for (int i = 2; i < INPUT_NEURONS_COUNT; i++)
			this->_inNeurons.emplace_back(new Neuron(i));
		this->_neurons.reserve(middleLayerSize);
		for (int i = 0; i < middleLayerSize; i++)
			this->_neurons.emplace_back(new GenNeuron());
		this->_outNeurons.reserve(8);
		for (int i = 0; i < 8; i++)
			this->_outNeurons.emplace_back(new GenNeuron());
	}

	void GeneticAI::_generateLinks()
	{
		for (auto &gene : this->_genome) {
			auto *neurOut = reinterpret_cast<GenNeuron *>(this->getOutNeuron(gene));
			auto *neurIn = this->getInNeuron(gene);

			if (!neurOut || !neurIn)
				continue;
			neurOut->addLink(
				gene.weight / (INT16_MAX / 4.f),
				gene.add    / (INT16_MAX / 4.f),
				*neurIn
			);
		}
	}

	int GeneticAI::_calculatePalette()
	{
		unsigned sum = 0;

		for (auto &gene : this->_genome)
			for (auto data : gene.data)
				sum += data;
		return sum % 74;
	}

	Neuron *GeneticAI::getInNeuron(const Gene &gene) const
	{
		auto &arr = (gene.isInput ? this->_inNeurons : this->_neurons);
		auto resultId = gene.neuronIdIn % arr.size();

		return &*arr[resultId];
	}

	Neuron *GeneticAI::getOutNeuron(const Gene &gene)
	{
		auto &arr = (gene.isOutput ? this->_outNeurons : this->_neurons);
		auto resultId = gene.neuronIdOut % arr.size();

		return &*arr[resultId];
	}
}