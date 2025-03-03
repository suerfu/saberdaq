import ROOT
import random

# Create a ROOT file
file = ROOT.TFile("example.root", "RECREATE")

# Create a TTree
tree = ROOT.TTree("myTree", "A simple TTree with multiple branches")

# Define variables and link them to the tree
x = ROOT.std.vector('float')()
y = ROOT.std.vector('float')()
z = ROOT.std.vector('int')()

tree.Branch("x", x)
tree.Branch("y", y)
tree.Branch("z", z)

# Fill the tree with random data
for i in range(100):  # 100 entries
    x.clear()
    y.clear()
    z.clear()
    
    x.push_back(random.uniform(0, 10))  # Random float [0,10]
    y.push_back(random.gauss(5, 2))     # Gaussian with mean 5, std 2
    z.push_back(random.randint(0, 100)) # Random integer [0,100]
    
    tree.Fill()

# Write the tree to the file
file.Write()
file.Close()

print("ROOT file 'example.root' created with TTree 'myTree' containing multiple branches")
