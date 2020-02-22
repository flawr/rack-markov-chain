# Markov Chain Module 

This is a 4-state [Markov Chain](https://en.wikipedia.org/wiki/Markov_chain) module for [VCV Rack](https://vcvrack.com/).

![](/patches/two-mc-patch.png)

## Use
The Module has four states: For each state you can define transition probabilities to all states. Each column in the 4x4 matrix represents the probabilities for the transition to the other states. In each step a new state is chose according to these probabilities. (Note that since the probabilities have to add up to 1.0, the values of each column are normalized: For instance when you set all knobs of one state to 0.1, the probabilities for the next state are actually each 0.1/(0.1+0.1+0.1+0.1) = 0.25. Each 

## Installation
To install it, download the zip-file from the `dist` folder and copy it to the [appropriate plugin folder](https://vcvrack.com/manual/Installing#installing-plugins-not-available-on-the-plugin-manager).
You can also follow [this tutorial](https://vcvrack.com/manual/PluginDevelopmentTutorial) to build it yourself.
