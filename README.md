# Tiny ML - Weather Predictions

** Latest update 4/15/2024 **

In this repository, I collect my very first experiments with tinyML on an Arduino board of sensors. First, I build a TensorFlow neural network model trained on public weather [data](https://www.ncei.noaa.gov/access/crn/qcdatasets.html) collected from the National Centers for Environmental Information (NOAA) site.  Second, I convert the TensorFlow model to a TFlite model for deployment to the Arduino board. Finally, I develop an iOS app that runs on my iPhone and is connected to the board via Bluetooth. The app visualizes live weather data and the corresponding predictions from my model. The repository is structured as follows: 

tinyml-weather/

├── arduino/ [the code that is set up on the Arduino board]

├── iosapp/ [the code to deploy the app to an iPhone]

├── notebooks/ [Jupyter notebook for the data preparation and model building] 

The pyproject.toml contains the packages I used to create a Poetry environment to run the code. Notice that I sed a MacBook to work on this project. You may need to adapt the packages to your own OS. For example, since my MacBook runs a M1 chip, the Tensorflow library that I used is tensorflow-macos.

Feel free to [drop me](mailto:alessioai@icloud.com?subject=tinyml-weather%20inquiry) a line if you are interested in collaborating on this project.

