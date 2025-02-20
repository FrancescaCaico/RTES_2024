import org.omnetpp.queueing.Queue;
import org.omnetpp.queueing.Sink;
import org.omnetpp.queueing.Source;
import org.omnetpp.queueing.Router;

//  DELAY --> IN[], OUT 
//  SOURCE --> OUT
//  SINK --> IN[]
//  QUEUE --> IN[] , OUT
//  ROUTER --> IN[] , OUT []


network exam {
    parameters:
        int N = default(20);
        double mu = default(10); 
        double lambda = default(100);
        double cv = default(3);
        src.interArrivalTime = 1s * exponential(1/lambda); 
        srv[*].serviceTime =1s * lognormal(log(1.0/(mu*sqrt(1+cv^2))), sqrt(log(1+cv^2)));
        srv[*].capacity = -1;
        LoadBalancer.routingAlgorithm = "roundRobin";
    submodules:
        src: Source; 
        srv[N]: Queue; 
        sink : Sink;
        LoadBalancer: Router;
    connections:
        src.out --> LoadBalancer.in++;  
        for i=0..N-1 {
            LoadBalancer.out++ --> srv[i].in++;
            srv[i].out --> sink.in++;
        }
        
}
  