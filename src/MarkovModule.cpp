#include "plugin.hpp"


struct MarkovModule : Module {
	enum ParamIds {
		CLOCK_PARAM,
		PROB00_PARAM,
		PROB10_PARAM,
		PROB20_PARAM,
		PROB30_PARAM,
		PROB01_PARAM,
		PROB11_PARAM,
		PROB21_PARAM,
		PROB31_PARAM,
		PROB02_PARAM,
		PROB12_PARAM,
		PROB22_PARAM,
		PROB32_PARAM,
		PROB03_PARAM,
		PROB13_PARAM,
		PROB23_PARAM,
		PROB33_PARAM,
		VOL0_PARAM,
		VOL1_PARAM,
		VOL2_PARAM,
		VOL3_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE0_OUTPUT,
		GATE1_OUTPUT,
		GATE2_OUTPUT,
		GATE3_OUTPUT,
		GATE_TOTAL_OUTPUT,
		CV_TOTAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		CLOCK_LIGHT,
		BLINK0_LIGHT,
		BLINK1_LIGHT,
		BLINK2_LIGHT,
		BLINK3_LIGHT,
		NUM_LIGHTS
	};

	MarkovModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CLOCK_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB00_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB10_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB20_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB30_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB01_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB11_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB21_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB31_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB02_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB12_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB22_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB32_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB03_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB13_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB23_PARAM, 0.f, 1.f, 0.f, "");
		configParam(PROB33_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VOL0_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VOL1_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VOL2_PARAM, 0.f, 1.f, 0.f, "");
		configParam(VOL3_PARAM, 0.f, 1.f, 0.f, "");
	}
    
    int vol[4] = {VOL0_PARAM, VOL1_PARAM, VOL2_PARAM, VOL3_PARAM};
    int gate[4] = {GATE0_OUTPUT, GATE1_OUTPUT, GATE2_OUTPUT, GATE3_OUTPUT};
    int blink[4] = {BLINK0_LIGHT, BLINK1_LIGHT, BLINK2_LIGHT, BLINK3_LIGHT};
    int prob[4][4] = {
        {PROB00_PARAM, PROB01_PARAM, PROB02_PARAM, PROB03_PARAM},
        {PROB10_PARAM, PROB11_PARAM, PROB12_PARAM, PROB13_PARAM},
        {PROB20_PARAM, PROB21_PARAM, PROB22_PARAM, PROB23_PARAM},
        {PROB30_PARAM, PROB31_PARAM, PROB32_PARAM, PROB33_PARAM},
    };
    float prop_cdf[4];

    float phase = 0.f; //between -0.5f and +0.5f
    int state = 0;
    const int STATES = 4;
    unsigned int rng_state = 0;
    unsigned int rng_multiplier = 1103515245;
    unsigned int rng_increment = 12345;
    unsigned int rng_modulus = 2147483648; // 2^31
    const float INPUT_THRESHOLD = 1.f;
    const float GATE_VOLTAGE = 5.f;
    const float CV_VOLTAGE = 5.f;
    float clock_last = 0.f;
    

	void process(const ProcessArgs& args) override {
        // CLOCK
        float clock_param = params[CLOCK_PARAM].getValue();
        float freq = std::pow(1000.f + 1.0f, clock_param) - 1.0f;
        phase += freq * args.sampleTime;
        if (phase >= 0.5f)
        {
            phase -= 1.f;
        }
        float clock_input = 0.f;
        if (inputs[CLOCK_INPUT].isConnected())
        {
            clock_input = inputs[CLOCK_INPUT].getVoltage();
        } else {
            clock_input = 10.0f * phase; // map phase [-.5, .5] to [-5, 5]
        }
        float clock_thresh = clock_input > INPUT_THRESHOLD ? 1.f : 0.f; //this should be used for further processing
        lights[CLOCK_LIGHT].setBrightness(clock_thresh);
        
        //READ probabilites from knobs
        float current = 0.f;
        float total = 0.f;
        for(int k=0; k < STATES; k++)
        {
            current = params[prob[state][k]].getValue();
            total += current;
            prop_cdf[k] = total;
        }
        // RANDOM number generation
        rng_state = (rng_multiplier * rng_state + rng_increment) % rng_modulus; 
        float random = (float) rng_state / (float) rng_modulus;
        
        // SAMPLE from cdf of current state
        int new_state = 0;
        for(int k = STATES - 1; k >= 0; k--)
        {
            if (random * total <= prop_cdf[k])
            {
                new_state = k;
            }
        }
               
        
        //STEP
        if (clock_last < clock_thresh)
        {
            //now we ACTUALLY are transitioning from one step to the next
            state = new_state;
        }
        clock_last = clock_thresh;
        
        for(int k=0; k < STATES; k++)
        {
            //update or reste 
            if (k == state)
            {
                lights[blink[k]].setBrightness(clock_thresh);
                float cv_voltage = CV_VOLTAGE * params[vol[state]].getValue() * 2.f - 1.f;
                // map to [-CV_VOLTAGE, CV_VOLTAGE]
                outputs[CV_TOTAL_OUTPUT].setVoltage(cv_voltage); 
                outputs[gate[k]].setVoltage(GATE_VOLTAGE * clock_thresh);
                outputs[GATE_TOTAL_OUTPUT].setVoltage(GATE_VOLTAGE * clock_thresh);
            } 
            else
            {
                lights[blink[k]].setBrightness(0.f);
                outputs[gate[k]].setVoltage(0.f);
            }
        }        
	}
};


struct MarkovModuleWidget : ModuleWidget {
	MarkovModuleWidget(MarkovModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MarkovModule.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 20.973)), module, MarkovModule::CLOCK_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.441, 34.414)), module, MarkovModule::PROB00_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 34.414)), module, MarkovModule::PROB10_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.323, 34.414)), module, MarkovModule::PROB20_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(53.763, 34.414)), module, MarkovModule::PROB30_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.441, 47.855)), module, MarkovModule::PROB01_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 47.855)), module, MarkovModule::PROB11_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.323, 47.855)), module, MarkovModule::PROB21_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(53.763, 47.855)), module, MarkovModule::PROB31_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.441, 61.296)), module, MarkovModule::PROB02_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 61.296)), module, MarkovModule::PROB12_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.323, 61.296)), module, MarkovModule::PROB22_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(53.763, 61.296)), module, MarkovModule::PROB32_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.441, 74.737)), module, MarkovModule::PROB03_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 74.737)), module, MarkovModule::PROB13_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.323, 74.737)), module, MarkovModule::PROB23_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(53.763, 74.737)), module, MarkovModule::PROB33_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(13.441, 121.78)), module, MarkovModule::VOL0_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(26.882, 121.78)), module, MarkovModule::VOL1_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(40.323, 121.78)), module, MarkovModule::VOL2_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(53.763, 121.78)), module, MarkovModule::VOL3_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(13.441, 20.973)), module, MarkovModule::CLOCK_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(13.441, 96.242)), module, MarkovModule::GATE0_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(26.882, 96.242)), module, MarkovModule::GATE1_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(40.323, 96.242)), module, MarkovModule::GATE2_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.763, 96.242)), module, MarkovModule::GATE3_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(67.204, 96.242)), module, MarkovModule::GATE_TOTAL_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(67.204, 121.78)), module, MarkovModule::CV_TOTAL_OUTPUT));

		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(40.323, 20.973)), module, MarkovModule::CLOCK_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(13.441, 108.339)), module, MarkovModule::BLINK0_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(26.882, 108.339)), module, MarkovModule::BLINK1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(40.323, 108.339)), module, MarkovModule::BLINK2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(53.763, 108.339)), module, MarkovModule::BLINK3_LIGHT));
	}
};


Model* modelMarkovModule = createModel<MarkovModule, MarkovModuleWidget>("MarkovModule");