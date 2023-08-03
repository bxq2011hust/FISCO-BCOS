cd /data/chaos/java-sdk-demo/dist
java -cp 'conf/:lib/*:apps/*' org.fisco.bcos.sdk.demo.perf.PerformanceTransferDMC group0 1 5000 500 >nohup_java.out 2>&1
cat nohup_java.out | grep "Check OK" | wc -l 
cd - > out
