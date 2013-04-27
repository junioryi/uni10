#include "SyTensor.h"
#include "Network.h"
Node_t::Node_t(): T(NULL), elemNum(0), parent(NULL), left(NULL), right(NULL){
}
Node_t::Node_t(SyTensor_t* Tp): T(Tp), elemNum(Tp->elemNum), labels(Tp->labels), bonds(Tp->bonds), parent(NULL), left(NULL), right(NULL){	
	assert(Tp->status & INIT);
	assert(Tp->status & HAVELABEL);
}
Node_t::Node_t(const Node_t& nd): T(nd.T), elemNum(nd.elemNum), labels(nd.labels), bonds(nd.bonds), parent(nd.parent), left(nd.left), right(nd.right){	
}
Node_t::Node_t(vector<Bond_t>& _bonds, vector<int>& _labels): T(NULL), labels(_labels), bonds(_bonds), parent(NULL), left(NULL), right(NULL){	
	elemNum = cal_elemNum(bonds);
}
Node_t::~Node_t(){
	cout<<"DELETE Node......\n";
}
Node_t Node_t::contract(Node_t* nd){
	int AbondNum = bonds.size();
	int BbondNum = nd->bonds.size();
	vector<Bond_t> cBonds;
	vector<int> markB(BbondNum, 0);
	vector<int> newLabelC;
	int conBondNum = 0;
	bool match;
	for(int a = 0; a < AbondNum; a++){
		match = false;
		for(int b = 0; b < BbondNum; b++)
			if(labels[a] == nd->labels[b]){
				markB[b] = 1;
				match = true;
				conBondNum++;
				break;
			}
		if(!match){
			newLabelC.push_back(labels[a]);
			cBonds.push_back(bonds[a]);
		}
	}
	for(int b = 0; b < BbondNum; b++)
		if(markB[b] == 0){
			newLabelC.push_back(nd->labels[b]);
			cBonds.push_back(nd->bonds[b]);
		}
	int rBondNum = AbondNum - conBondNum;
	int cBondNum = BbondNum - conBondNum;
	for(int a = 0; a < rBondNum; a++)
		cBonds[a].change(BD_ROW);
	for(int a = 0; a < cBondNum; a++)
		cBonds[rBondNum + a].change(BD_COL);

	Node_t par(cBonds, newLabelC);
	return par;
}

float Node_t::metric(Node_t* nd){	//Bigger is better
	int AbondNum = bonds.size();
	int BbondNum = nd->bonds.size();
	vector<Bond_t> cBonds;
	vector<int> markB(BbondNum, 0);
	int conBondNum = 0;
	bool match;
	for(int a = 0; a < AbondNum; a++){
		match = false;
		for(int b = 0; b < BbondNum; b++)
			if(labels[a] == nd->labels[b]){
				markB[b] = 1;
				match = true;
				conBondNum++;
				break;
			}
		if(!match)
			cBonds.push_back(bonds[a]);
	}
	if(conBondNum == 0)
		return -1;
	for(int b = 0; b < BbondNum; b++)
		if(markB[b] == 0)
			cBonds.push_back(nd->bonds[b]);
	int rBondNum = AbondNum - conBondNum;
	int cBondNum = BbondNum - conBondNum;
	for(int a = 0; a < rBondNum; a++)
		cBonds[a].change(BD_ROW);
	for(int a = 0; a < cBondNum; a++)
		cBonds[rBondNum + a].change(BD_COL);
	int64_t newElemNum = cal_elemNum(cBonds);
	return float(elemNum + nd->elemNum) / newElemNum;
}

int64_t Node_t::cal_elemNum(vector<Bond_t>& _bonds){
	int rBondNum = 0;
	int cBondNum = 0;
	for(int b = 0; b < _bonds.size(); b++)
		if(_bonds[b].type == BD_ROW)
			rBondNum++;
		else if(_bonds[b].type == BD_COL)
			cBondNum++;
	Qnum_t qnum(0, 0);
	int dim;
	map<Qnum_t,int> row_QnumMdim;
	vector<int> row_offs(rBondNum, 0);
	if(rBondNum){
		while(1){
			qnum.set();
			dim = 1;
			for(int b = 0; b < rBondNum; b++){
				qnum = qnum * _bonds[b].Qnums[row_offs[b]];
				dim *= _bonds[b].Qdegs[row_offs[b]];
			}
			if(row_QnumMdim.find(qnum) != row_QnumMdim.end())
				row_QnumMdim[qnum] += dim;
			else
				row_QnumMdim[qnum] = dim;
			int bidx;
			for(bidx = rBondNum - 1; bidx >= 0; bidx--){
				row_offs[bidx]++;
				if(row_offs[bidx] < _bonds[bidx].Qnums.size())
					break;
				else
					row_offs[bidx] = 0;
			}
			if(bidx < 0)	//run over all row_bond offsets
				break;
		}
	}
	else{
		qnum.set();
		row_QnumMdim[qnum] = 1;
	}

	map<Qnum_t,int> col_QnumMdim;
	vector<int> col_offs(cBondNum, 0);
	if(cBondNum){
		while(1){
			qnum.set();
			dim = 1;
			for(int b = 0; b < cBondNum; b++){
				qnum = qnum * _bonds[b + rBondNum].Qnums[col_offs[b]];
				dim *= _bonds[b + rBondNum].Qdegs[col_offs[b]];
			}
			if(row_QnumMdim.find(qnum) != row_QnumMdim.end()){
				if(col_QnumMdim.find(qnum) != col_QnumMdim.end())
					col_QnumMdim[qnum] += dim;
				else
					col_QnumMdim[qnum] = dim;
			}
			int bidx;
			for(bidx = cBondNum - 1; bidx >= 0; bidx--){
				col_offs[bidx]++;
				if(col_offs[bidx] < _bonds[bidx + rBondNum].Qnums.size())
					break;
				else
					col_offs[bidx] = 0;
			}
			if(bidx < 0)	//run over all row_bond offsets
				break;
		}
	}
	else{
		qnum.set();
		if(row_QnumMdim.find(qnum) != row_QnumMdim.end())
			col_QnumMdim[qnum] = 1;
	}
	int64_t _elemNum = 0;
	map<Qnum_t,int>::iterator it;
	map<Qnum_t,int>::iterator it2;
	for ( it2 = col_QnumMdim.begin() ; it2 != col_QnumMdim.end(); it2++ ){
		it = row_QnumMdim.find(it2->first);
		_elemNum += it->second * it2->second;
	}
	return _elemNum;
}

Network_t::Network_t(): root(NULL), times(0), tot_elem(0), max_elem(0){
}

Network_t::Network_t(vector<SyTensor_t*>& tens): root(NULL), times(0), tot_elem(0), max_elem(0){
	for(int i = 0; i < tens.size(); i++)
		add(tens[i]);
}

Node_t* Network_t::add(SyTensor_t* SyTp){
	Node_t* ndp = new Node_t(SyTp);
	order.push_back(leafs.size());
	leafs.push_back(ndp);
	return ndp;
}

void Network_t::branch(Node_t* sbj, Node_t* tar){
	Node_t* par = new Node_t(sbj->contract(tar));
	if(tar->parent != NULL)	//tar is not root
		if(tar->parent->left == tar)	// tar on the left side of its parent
			tar->parent->left = par;
		else
			tar->parent->right = par;
	par->parent = tar->parent;
	par->left = sbj;
	par->right = tar;
	sbj->parent = par;
	tar->parent = par;
}
void Network_t::matching(Node_t* sbj, Node_t* tar){
	if(tar == NULL)	//tar is root
		root = sbj;
	else if(tar->T == NULL){	//not leaf
		float sbj_p;
		if((sbj_p = sbj->metric(tar)) > 0){	//has contracted bonds
			assert(tar->left != NULL && tar->right != NULL);
			float lft_p, rht_p;
			if((lft_p = sbj->metric(tar->left)) > sbj_p || (rht_p = sbj->metric(tar->right)) > sbj_p){	//go deeper comparison to the children
				if(lft_p > rht_p)
					matching(sbj, tar->left);
				else
					matching(sbj, tar->right);
			}
			else	//contract!!!
				branch(sbj, tar);
		}
		else	//contract!!!
			branch(sbj, tar);
	}
	else	//contract!!!
		branch(sbj, tar);
}

void Network_t::construct(){
	for(int i = 0; i < order.size(); i++){
		cout<<order[i]<<endl;
		matching(leafs[order[i]], root);
	}
	for(int i = 0; i < order.size(); i++)
		cout<<(*(leafs[order[i]]));
}

void Network_t::optimize(int num){
	if(times == 0)
		construct();
	
}

Network_t::~Network_t(){
	for(int i = 0; i < leafs.size(); i++)
		delete leafs[i];
}

ostream& operator<< (ostream& os, const Node_t& nd){
	os << "Tensor: " << nd.T<<endl;
	os << "elemNum: " << nd.elemNum<<endl;
	os << "parent: " << nd.parent<<endl;
	os << "left: " << nd.left<<endl;
	os << "right: " << nd.right<<endl;
	os << "labels: ";
	for(int i = 0; i < nd.labels.size(); i++)
		os << nd.labels[i] << ", ";
	os << endl;
	for(int i = 0; i < nd.bonds.size(); i++)
		os << "    " <<  nd.bonds[i];
	return os;
}
