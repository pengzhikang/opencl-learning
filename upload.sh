if [ ! -d ~/project/github/opencl-learning ]; then
    mkdir ~/project/github/opencl-learning
fi
cp -rf ./* ~/project/github/opencl-learning/
cd ~/project/github/opencl-learning
for file in ./*:
do
    if [ -d $file ]; then
        if [ -d $file/build]; then
            rm -rf $file/build
        fi
    fi
done

git add *
git commit -m $0
git push -u origin main
        