#include "graph.h"

const char* fieldGraph= "Graph";

/*nodes consts*/
const char* fieldNode= "Node";
const char* fieldId= "Id";
const char* fieldName= "Name";
const char* fieldNumber= "Number";
const char* fieldFloor= "Floor";
const char* fieldClassA= "ClassA";
const char* fieldClassB= "ClassB";
const char* fieldClassC= "ClassC";
const char* fieldClassD= "ClassD";
const char* fieldClassE= "ClassE";

/*edges consts*/
const char* fieldEdge= "Edge";
const char* fieldWeight= "Weight";
const char* fieldType= "Type";
const char* fieldRegular= "Regular";
const char* fieldElevator= "Elevator";
const char* fieldStairs= "Stairs";
const char* fieldNode1Id= "Node1Id";
const char* fieldNode2= "Node2Id";
const char* fieldVideo= "video";
const char* fieldFromId= "fromId";
const char* fieldToId= "toId";
const char* fieldVideoStart= "videoStart";
const char* fieldVideoEnd= "videoEnd";
const char* fieldPathTovideo= "pathTovideo";



map<string, EdgeType> typeMap = {{ fieldRegular, Regular },{ fieldElevator, Elevator },{ fieldStairs, Stairs } };

using namespace rapidxml;

Graph::Graph(string xmlPath, bool& succesReadingXmls)
{

    succesReadingXmls = ParseXmlNodes(xmlPath);
    succesReadingXmls &= ParseXmlEdges(xmlPath);
}
Graph::~Graph()
{
    delete(_nodes);
    delete(_edges);
}

bool Graph::ParseXmlNodes(string xmlPathNodes)
{
    xml_document<> doc;
    xml_node<> * root;
    // Read the xml file into a vector
    ifstream xmlFile(xmlPathNodes);
    vector<char> buffer((istreambuf_iterator<char>(xmlFile)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    doc.parse<0>(&buffer[0]);
    // Find our root
    root = doc.first_node(fieldGraph);
    if (!root) return 0;

    // Iterate over the nodes
    _nodes = new list<Node*>();
    for (xml_node<> * vertex_node = root->first_node(fieldNode); vertex_node && strcmp(vertex_node->name(), fieldNode) == 0; vertex_node = vertex_node->next_sibling())
    {
        int id = atoi(vertex_node->first_attribute(fieldId)->value());
        string name = vertex_node->first_attribute(fieldName)->value();
        string number = vertex_node->first_attribute(fieldNumber)->value();
        int floor = atoi(vertex_node->first_attribute(fieldFloor)->value());
        int classes[NUMBER_OF_CLASSES];
        classes[0] = atoi(vertex_node->first_attribute(fieldClassA)->value());
        classes[1] = atoi(vertex_node->first_attribute(fieldClassB)->value());
        classes[2] = atoi(vertex_node->first_attribute(fieldClassC)->value());
        classes[3] = atoi(vertex_node->first_attribute(fieldClassD)->value());
        classes[4] = atoi(vertex_node->first_attribute(fieldClassE)->value());
        int howManyClassesFound = 0;
        for(int cl=0; cl<NUMBER_OF_CLASSES; ++cl){
            if (classes[cl] >0)//just a counter for loops to come
                howManyClassesFound++;
            if (classes[cl] <= 0) //after first 0, all will be 0
                break;
        }
        Node *node = new Node(id, name,number, floor,classes,howManyClassesFound);
        _nodes->push_back(node);
    }
    return 1;
}

bool Graph::ParseXmlEdges(string xmlPathEdges)
{
    xml_document<> doc;
    xml_node<> * root;
    // Read the xml file into a vector
    ifstream xmlFile(xmlPathEdges);
    vector<char> buffer((istreambuf_iterator<char>(xmlFile)), istreambuf_iterator<char>());
    buffer.push_back('\0');
    // Parse the buffer using the xml file parsing library into doc
    doc.parse<0>(&buffer[0]);
    // Find our root
    root = doc.first_node(fieldGraph);
    if (!root) return 0;


    // Iterate over the nodes
    _edges = new list<Edge*>();
    for (xml_node<> * vertex_node = root->first_node(fieldEdge); vertex_node && strcmp(vertex_node->name(), fieldEdge) == 0; vertex_node = vertex_node->next_sibling())//#mark add strcmp like above function
    {
        double weight = atof(vertex_node->first_attribute(fieldWeight)->value());
        string typeStr = vertex_node->first_attribute(fieldType)->value();
        EdgeType type = typeMap.find(typeStr)->second;
        int nodeId1 = atoi(vertex_node->first_attribute(fieldNode1Id)->value());
        int nodeId2 = atoi(vertex_node->first_attribute(fieldNode2)->value());
        Node* node1 = NULL;
        Node* node2 = NULL;
        //todo - make a container on ParseXmlNodes() that will do mapping: < node->id, Node* >
        //use it here instead of going over all nodes which could be very BAD if the building has a lot of nodes
        for (Node* node : *_nodes)//to get the Nodes objects iterate through all nodes.
        {
            if (node->GetId() == nodeId1)
            {
                node1 = node;
            }
            else if (node->GetId() == nodeId2)
            {
                node2 = node;
            }
        }
        Edge *edge = new Edge(weight,node1,node2,type);
        //first video goes from minId to MaxId. second if from max to min
        int videoIndex = 1;
        int minId = (nodeId1 < nodeId2) ? nodeId1 : nodeId2;
        int maxId = (nodeId1 > nodeId2) ? nodeId1 : nodeId2;
        for (xml_node<> * video_node = vertex_node->first_node(fieldVideo); video_node&& strcmp(vertex_node->name(), fieldEdge) == 0; video_node = video_node->next_sibling())
        {
            int fromId = (videoIndex==1) ? minId : maxId;
            int toId   = (videoIndex==1) ? maxId : minId;
            int videoStart = atoi(video_node->first_attribute(fieldVideoStart)->value());
            int videoEnd = atoi(video_node->first_attribute(fieldVideoEnd)->value());
            string pathTovideo = video_node->first_attribute(fieldPathTovideo)->value();
            videoInfo vidInfo(fromId, toId, videoStart, videoEnd, pathTovideo);
            edge->SetVideoInfo(videoIndex++,vidInfo);
        }
        _edges->push_back(edge);
    }
    return 1;
}

list<Node*> Graph::GetGrapghNodes() const
{
    return *_nodes;
}
list<Edge*> Graph::GetGrapghEdges() const
{
    return *_edges;
}

list<Node *> Graph::GetShortestpath(Node* start, Node* end, EdgeType pref)
{
    bool wasChecked[MAXNODES];
    int distanceV[MAXNODES];
    vector< qPair > Adjacency[MAXNODES];
    priority_queue< qPair, vector< qPair >, comp > Queue;


    // initizlized adjacency container
	for (Edge* edge : *_edges)
    {
        int prefAddon = 0;
        Node* u = edge->GetNode1();
        Node* v = edge->GetNode2();
        double weight = edge->GetWeight();
        EdgeType edgeType = edge->GetEdgeType();
        if (edgeType != EdgeType::Regular){
            if(pref == EdgeType::Stairs && edgeType == EdgeType::Elevator ){
                prefAddon = INF;
            }
            else if(pref == EdgeType::Elevator && edgeType == EdgeType::Stairs ){
                prefAddon = INF;
            }
        }
        Adjacency[u->GetId()].push_back(qPair(v, weight + prefAddon));
        Adjacency[v->GetId()].push_back(qPair(u, weight + prefAddon));
    }


	// initialize distance vector
    for (unsigned int i = 1; i <= _nodes->size(); i++) {
        distanceV[i] = INFVALUE;
        wasChecked[i]=false;
    }
    distanceV[end->GetId()] = 0;
    Queue.push(qPair(end, 0));


    // dijkstra
    while (!Queue.empty())
    {
        Node* u = Queue.top().first;
		Queue.pop();

        if (u == start)
		{
			break;
        }
		if (wasChecked[u->GetId()]) continue;
        int mySize = Adjacency[u->GetId()].size();
        for (int i = 0; i < mySize; i++) {
            Node* v = Adjacency[u->GetId()][i].first;
            double weight = Adjacency[u->GetId()][i].second;
            if (!wasChecked[v->GetId()] && distanceV[u->GetId()] + weight < distanceV[v->GetId()]) {
				distanceV[v->GetId()] = distanceV[u->GetId()] + weight;
				v->SetPreviosNode(u);
				v->SetEdgeWeightToPrevious(weight);				
				Queue.push(qPair(v, distanceV[v->GetId()]));
			}
		}
		wasChecked[u->GetId()] = 1; // done with u
	}

    //save the rooms Nodes
    list<Node*>* shortestNodesPath = new list<Node*>;
	// push to list the starting room
    start->setdistanceToNextNodeInPath(start->GetEdgeWeightToPrevious());
    start->setnextRoomInPathId(start->GetPreviosNode()->GetId());
    start->resetCounter();
    start->saveVideoInfoOfNodesInPath( getVideoInfo(start) );

    shortestNodesPath->push_back(start);
    Node *next = start->GetPreviosNode();
    while (next != NULL && next != end)
    {
		if (next->GetPreviosNode() != NULL)
        {
            next->setdistanceToNextNodeInPath(next->GetEdgeWeightToPrevious());
            next->setnextRoomInPathId(next->GetPreviosNode()->GetId());
            next->resetCounter();
            next->saveVideoInfoOfNodesInPath( getVideoInfo(next) );
            shortestNodesPath->push_back(next);
		}
		next = next->GetPreviosNode();
	}


	//push to the list the ending room
    end->setdistanceToNextNodeInPath(0);
    end->setnextRoomInPathId(-1);
    end->resetCounter();
    end->saveVideoInfoOfNodesInPath(videoInfo(-1,-1,-1,-1,"-1")); //empty video info. end doesn't have from video
    shortestNodesPath->push_back(end);
    // nothing to shrink if there is only one destination on the path
    return (shortestNodesPath->size() < 2) ? (*shortestNodesPath) :GetShrinkendShortestPath(*shortestNodesPath);
}

list<Node *> Graph::GetShrinkendShortestPath(list<Node *> shortestPath)
{
    list<Node*>* shrinkedShortestPathtmp = new list<Node*>;
    list<Node*>::iterator it1 = shortestPath.begin();
    list<Node*>::iterator it2 = shortestPath.begin();
    list<Node*>::iterator itEnd = shortestPath.end();
    ++it2;
    while(it2!= itEnd)
    {
//        string name1 = (*it1)->GetName();
//        string name2 = (*it2)->GetName();
//        string name22 = "";
        int sharedClass = 0;
        int sharedClassLocal = 0;

        sharedClassLocal = findSameClass((*it1), (*it2), sharedClass);
        while(sharedClassLocal!=0 && sharedClass == sharedClassLocal){
            list<Node*>::iterator tmpNodeIter = it2;//tmpNodeIter will hold the next node. we will advance it2 to next only if we joined the node legaly
            ++tmpNodeIter;
//if(tmpNodeIter!= itEnd) name22 = (*tmpNodeIter)->GetName();
            sharedClassLocal = (tmpNodeIter!= itEnd ) ? findSameClass((*it1), *tmpNodeIter, sharedClass) : 0;
            if(sharedClassLocal!=0 && sharedClass == sharedClassLocal ){
                (*it1)->addTodistanceToNextNodeInPath((*it2)->distanceToNextNodeInPath());
                (*it1)->saveVideoInfoOfNodesInPath((*it2)->getMyVideoInfo());
                it2++;
            }
        }
        (*it1)->setnextRoomInPathId((*it2)->GetId());
        shrinkedShortestPathtmp->push_back((*it1));
        it1 = it2;
        ++it2;
    }
    shortestPath.clear();
    return *shrinkedShortestPathtmp;
}


int Graph::findSameClass(Node *a, Node *b, int &sharedClass)
{
    if(a->howManyClassesFound()==0 || b->howManyClassesFound()==0) return 0;
    const int * classes1 = a->GetClasses();
    const int * classes2 = b->GetClasses();
    for(int cl=0; cl < a->howManyClassesFound(); ++cl)
    {
        for(int cl2=0; cl2 < b->howManyClassesFound(); ++cl2)
        {
            if(classes1[cl] == classes2[cl2])
            {
                int sharedClassLocal = classes1[cl];
                if(sharedClass == 0)
                {
                    sharedClass = sharedClassLocal;
                }
                return sharedClassLocal;
            }
        }
    }
    return 0;
}

videoInfo Graph::getVideoInfo(Node* node)
{
    for (Edge* edge : *_edges)
    {
        if (edge->GetNode1() == node || edge->GetNode2() == node)
        {
            for(int i=1; i<=2; ++i){
                videoInfo vidInfo = edge->GetVideoInfo(i);
                if(vidInfo._fromId == node->GetId() && vidInfo._toId == node->nextRoomInPathId())
                    return vidInfo;
            }
        }
    }
    return videoInfo(-1,-1,-1,-1,"-1");
}
