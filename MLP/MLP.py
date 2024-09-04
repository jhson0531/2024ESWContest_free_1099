import tensorflow as tf
import pandas as pd
import numpy as np

data_original = pd.read_csv('test_data.txt',sep = ' ', names = ['distance','velocity','acceleration','operation'])
data = pd.read_csv('test_data.txt',sep = ' ', names = ['distance','velocity','acceleration','operation'])
x_data = np.zeros((data.shape[0],3))
y_data = np.zeros((data.shape[0],1))
data_re = data.to_numpy()

for i in range ((data_re.shape[0])):
    for j in range (3):
        x_data[i][j] = data_re[i][j]
for i in range ((data_re.shape[0])):
    y_data[i][0] = data_re[i][3]

from sklearn.preprocessing import MinMaxScaler
from sklearn.preprocessing import StandardScaler
scaler = StandardScaler()
#scaler = MinMaxScaler()
scaler.fit(x_data)
x_data_re = scaler.transform(x_data)

from sklearn.preprocessing import OneHotEncoder

ohe = OneHotEncoder()
ohe.fit(y_data.reshape(-1,1))
y_data_re = ohe.transform(y_data.reshape(-1,1))

from sklearn.model_selection import train_test_split
x_train,x_test,y_train,y_test = train_test_split(x_data_re,y_data_re, test_size = 0.25, random_state = 20)

from sklearn.neural_network import MLPClassifier
MLP = MLPClassifier(hidden_layer_sizes = (10,10),  max_iter = 1000, activation = 'relu', solver ='adam')
MLP.fit(x_train,y_train)
y_pred = MLP.predict(x_test)
score = MLP.score(x_test,y_test)
print(score)

y_pred_re = ohe.inverse_transform(y_pred)

Nnode_layer = [3,10,10,2]
np.savetxt("Nnode_layer.txt",Nnode_layer,fmt = "%d")

np.savetxt("mean.txt",scaler.mean_,fmt = "%e")
np.savetxt("std.txt",scaler.scale_,fmt = "%e")

for i in range (3):
    a = "weight_" + str(i) + ".txt"
    np.savetxt(a,MLP.coefs_[i],fmt = "%.10e")

for i in range (3):
    a = "bias_" + str(i) + ".txt"
    np.savetxt(a,MLP.intercepts_[i],fmt = "%.10e")
