../build.linux/nachos -f
../build.linux/nachos -mkdir /aaaaaaaaa
../build.linux/nachos -mkdir /aaaaaaaaa/bbbbbbbbb
../build.linux/nachos -cp num_1000.txt f0_1000
../build.linux/nachos -mkdir /f
../build.linux/nachos -mkdir /aaaaaaaaa/bbbbbbbbb/ccccccccc
../build.linux/nachos -mkdir /aaaaaaaaa/bbbbbbbbb/ddddddddd
../build.linux/nachos -cp num_1000.txt /aaaaaaaaa/f1_1000
../build.linux/nachos -mkdir /aaaaaaaaa/bbbbbbbbb/ddddddddd/eeeeeeeee
../build.linux/nachos -cp num_100.txt /aaaaaaaaa/bbbbbbbbb/f2_100
../build.linux/nachos -cp num_1000000.txt /aaaaaaaaa/bbbbbbbbb/ddddddddd/f3_1000000
../build.linux/nachos -cp FS_test2 /aaaaaaaaa/bbbbbbbbb/ddddddddd/f4_test2
../build.linux/nachos -cp FS_test1 /aaaaaaaaa/bbbbbbbbb/ddddddddd/f5_test1
../build.linux/nachos -mkdir /aaaaaaaaa/bbbbbbbbb/ddddddddd/eeeeeeeee/ggggggggg
echo "======================= -l / ===============================" 
../build.linux/nachos -l /
echo "======================= -l /a ==============================" 
../build.linux/nachos -l /aaaaaaaaa
echo "======================= -l /a/b ============================" 
../build.linux/nachos -l /aaaaaaaaa/bbbbbbbbb
echo "======================= -l /a/b/d ==========================" 
../build.linux/nachos -l /aaaaaaaaa/bbbbbbbbb/ddddddddd
echo "======================= -lr / ==============================" 
../build.linux/nachos -lr /
echo "======================= -lr /a =============================" 
../build.linux/nachos -lr /aaaaaaaaa
echo "======================= -lr /a/b ===========================" 
../build.linux/nachos -lr /aaaaaaaaa/bbbbbbbbb
echo "======================= -lr /a/b/d =========================" 
../build.linux/nachos -lr /aaaaaaaaa/bbbbbbbbb/ddddddddd
echo "======================= print 1000 =========================" 
../build.linux/nachos -p /aaaaaaaaa/f1_1000
echo "======================= print 100 ==========================" 
../build.linux/nachos -p /aaaaaaaaa/bbbbbbbbb/f2_100
echo "======================= print 1000000 ======================" 
../build.linux/nachos -p /aaaaaaaaa/bbbbbbbbb/ddddddddd/f3_1000000
echo "======================= print \"pass ^^\" ==================" 
../build.linux/nachos -e /aaaaaaaaa/bbbbbbbbb/ddddddddd/f5_test1
../build.linux/nachos -e /aaaaaaaaa/bbbbbbbbb/ddddddddd/f4_test2
../build.linux/nachos -r /f0_1000
../build.linux/nachos -r /aaaaaaaaa/f1_1000
../build.linux/nachos -r /aaaaaaaaa/bbbbbbbbb/f2_100
../build.linux/nachos -r /aaaaaaaaa/bbbbbbbbb/ddddddddd/f4_test2
../build.linux/nachos -lr /
echo "============================================================" 
../build.linux/nachos -cp num_100.txt /f0_1000
../build.linux/nachos -cp num_1000.txt /aaaaaaaaa/f1_1000
../build.linux/nachos -cp num_100.txt /aaaaaaaaa/bbbbbbbbb/f2_100
../build.linux/nachos -cp FS_test2 /aaaaaaaaa/bbbbbbbbb/ddddddddd/f4_test2
../build.linux/nachos -rr /aaaaaaaaa/bbbbbbbbb/ddddddddd/f3_1000000
../build.linux/nachos -lr /
echo "======================= f4 e ===============================" 
../build.linux/nachos -l /aaaaaaaaa/bbbbbbbbb/ddddddddd
../build.linux/nachos -rr /aaaaaaaaa/bbbbbbbbb/
echo "============================================================" 
../build.linux/nachos -lr /
echo "============================================================" 
../build.linux/nachos -mkdir /aaaaaaaaa/afterRemove/
../build.linux/nachos -mkdir /aaaaaaaaa/afterRemove/congrats
../build.linux/nachos -l /aaaaaaaaa/afterRemove
echo "============================================================" 
../build.linux/nachos -lr /
echo "============================================================" 
../build.linux/nachos -rr /f
../build.linux/nachos -rr /f0_1000
../build.linux/nachos -rr /file1
../build.linux/nachos -rr /aaaaaaaaa