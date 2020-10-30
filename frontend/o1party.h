#pragma once
#include "Crypto/PRNG.h"
#include "Common/Defines.h"
#include "Common/Log.h"
#include "Common/Log1.h"
#include <set>
#include "gbf.h"

using namespace osuCrypto;


inline void party1_encode(std::vector<block> inputSet, const std::vector<block> aesKeys, std::vector<block>& okvsTable, u64 nParties, u64 type_okvs, u64 type_security)
{

	std::vector<block> setValues(inputSet.size(), ZeroBlock), hashInputSet(inputSet.size());
	std::vector<AES> vectorAES(nParties - 2); //for party 2 -> (n-1)
	std::vector <std::vector<block>> ciphertexts(nParties - 2); //ciphertexts[idxParty][idxItem]

	for (u64 i = 0; i < vectorAES.size(); ++i)
	{
		vectorAES[i].setKey(aesKeys[i]);
		ciphertexts[i].resize(inputSet.size());
	}
	if (type_security == secMalicious)
		mAesFixedKey.ecbEncBlocks(inputSet.data(), inputSet.size(), hashInputSet.data()); //H(xi)

	for (u64 i = 0; i < vectorAES.size(); ++i)
		if (type_security == secMalicious)
			vectorAES[i].ecbEncBlocks(hashInputSet.data(), inputSet.size(), ciphertexts[i].data()); //compute F_ki(H(xi))
		else
			vectorAES[i].ecbEncBlocks(inputSet.data(), inputSet.size(), ciphertexts[i].data()); //compute F_ki(xi)

	for (u64 idxItem = 0; idxItem < inputSet.size(); ++idxItem)
		for (u64 idxParty = 0; idxParty < ciphertexts.size(); ++idxParty)
			setValues[idxItem] = setValues[idxItem] ^ ciphertexts[idxParty][idxItem];

	//std::cout << IoStream::lock;
	//for (u64 i = 0; i < 2; i++)
	//	std::cout << setValues[i] << " - encode party 1 - " << i << std::endl;
	//std::cout << IoStream::unlock;

	if (type_okvs == GbfOkvs)
		GbfEncode(inputSet, setValues, okvsTable);
	else if (type_okvs == PolyOkvs)
		PolyEncode(inputSet, setValues, okvsTable);

	/*std::cout << IoStream::lock;
	for (u64 i = 0; i < 2; i++)
	{
		std::cout << okvsTable[i] << " - okvsTable - " << i << std::endl;
	}
	std::cout << IoStream::unlock;*/

	//if (type_okvs == PolyOkvs) //TODO
	//std::vector<block> inputSet2PSI(inputSet.size(), ZeroBlock);
	//GbfDecode(okvsTable, inputSet, inputSet2PSI); //Decode(okvsTable, x) where okvsTable is received from party 1
	//std::cout << IoStream::lock;
	//for (u64 i = 0; i < 2; i++)
	//{
	//	std::cout << inputSet2PSI[i] << " - setValues decode party 1 - " << i << std::endl;
	//}
	//std::cout << IoStream::unlock;
}

//for party 2->(n-2): returns okvsTable
inline void party2_encode(const std::vector<block> inputSet, const block& aesKey, std::vector<block>& okvsTable, u64 type_okvs, u64 type_security)
{

	std::vector<block> setValues(inputSet.size(), ZeroBlock), hashInputSet(inputSet.size());
	AES aes(aesKey);

	if (type_security == secMalicious)
		mAesFixedKey.ecbEncBlocks(inputSet.data(), inputSet.size(), hashInputSet.data()); //H(xi)

	if (type_security == secMalicious)
		aes.ecbEncBlocks(hashInputSet.data(), inputSet.size(), setValues.data()); //compute F_ki(H(xi))
	else
		aes.ecbEncBlocks(inputSet.data(), inputSet.size(), setValues.data()); //compute F_ki(xi)

	if (type_okvs == GbfOkvs)
		GbfEncode(inputSet, setValues, okvsTable);
	else if (type_okvs == PolyOkvs)
		PolyEncode(inputSet, setValues, okvsTable);

	//if (type_okvs == PolyOkvs) //TODO
}

//for party n-1: return A~_{n-1}
inline void partyn1_decode(const std::vector<block> inputSet, const block& aesKey, const std::vector <std::vector<block>> okvsTables, std::vector<block>& inputSet2PSI, u64 type_okvs, u64 type_security)
{
	inputSet2PSI.resize(inputSet.size(), ZeroBlock);
	std::vector<block> hashInputSet(inputSet.size());
	AES aes(aesKey);

	if (type_security == secMalicious)
		mAesFixedKey.ecbEncBlocks(inputSet.data(), inputSet.size(), hashInputSet.data()); //H(xi)

	if (type_security == secMalicious)
		aes.ecbEncBlocks(hashInputSet.data(), inputSet.size(), inputSet2PSI.data()); //compute F_ki(H(xi))
	else
		aes.ecbEncBlocks(inputSet.data(), inputSet.size(), inputSet2PSI.data()); //compute F_ki(xi)

	for (u64 idxParty = 0; idxParty < okvsTables.size(); idxParty++) //okvsTables[idxParty]
	{
		std::vector<block> setValues(inputSet.size());
		if (type_okvs == GbfOkvs)
			GbfDecode(okvsTables[idxParty], inputSet, setValues); // setValues[idxItem]=Decode(okvsTables[idxParty], x)
		else if (type_okvs == PolyOkvs)
			PolyDecode(okvsTables[idxParty], inputSet, setValues); // setValues[idxItem]=Decode(okvsTables[idxParty], x)


		for (u64 idxItem = 0; idxItem < inputSet.size(); ++idxItem) //compute xor all decode() 
			inputSet2PSI[idxItem] = inputSet2PSI[idxItem] ^ setValues[idxItem];
	}

	/*std::cout << IoStream::lock;
	for (u64 i = 0; i < 2; i++)
	{
		std::cout << inputSet2PSI[i] << " - inputSet2PSIn1 before last xor - " << i << std::endl;
	}
	std::cout << IoStream::unlock;*/

	for (u64 idxItem = 0; idxItem < inputSet.size(); ++idxItem)
		inputSet2PSI[idxItem] = inputSet2PSI[idxItem] ^ inputSet[idxItem]; //simulate x||F(x) xor all decodes

}


//for party n: return A~_n
inline void partyn_decode(const std::vector<block> inputSet, const std::vector<block> okvsTable, std::vector<block>& inputSet2PSI, u64 type_okvs, u64 type_security)
{
	inputSet2PSI.resize(inputSet.size(), ZeroBlock);
	std::vector<block> hashInputSet(inputSet.size());

	if (type_security == secMalicious)
		mAesFixedKey.ecbEncBlocks(inputSet.data(), inputSet.size(), hashInputSet.data()); //H(xi)

	if (type_okvs == GbfOkvs)
		GbfDecode(okvsTable, inputSet, inputSet2PSI); //Decode(okvsTable, x) where okvsTable is received from party 1
	else if (type_okvs == PolyOkvs)
		PolyDecode(okvsTable, inputSet, inputSet2PSI); //Decode(okvsTable, x) where okvsTable is received from party 1


	/*std::cout << IoStream::lock;
	for (u64 i = 0; i < 2; i++)
		std::cout << inputSet2PSI[i] << " - decode partyn - " << i << std::endl;
	std::cout << IoStream::unlock;*/

	for (u64 idxItem = 0; idxItem < inputSet.size(); ++idxItem)
		inputSet2PSI[idxItem] = inputSet2PSI[idxItem] ^ inputSet[idxItem]; //simulate x||F(x) xor all decodes
}

inline void party_test(u64 type_okvs)
{
	std::cout << " ============== party_test ==============\n";

	u64 nParties = 5, setSize = 128, intersection_size = 10;
	u64 party_n1 = nParties - 2; //party n-1
	u64 party_n = nParties - 1; //party n

	PRNG prng(_mm_set_epi32(4253465, 3434565, 234435, 23987045));
	std::vector <std::vector<block>> inputSets(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		inputSets[i].resize(setSize);
		for (u64 j = 0; j < setSize; ++j)
			inputSets[i][j] = prng.get<block>();
	}

	for (u64 i = 1; i < nParties; ++i) //same items
		for (u64 j = 0; j < intersection_size; ++j)
			inputSets[i][j] = inputSets[0][j];

	//generating aes keys
	std::vector<block> aesKeys(nParties - 2); //aesKeys[0] for party 2
	for (u64 i = 0; i < aesKeys.size(); ++i)
		aesKeys[i] = prng.get<block>();


	std::vector<block> okvsTable1; //okvs of party1
	party1_encode(inputSets[0], aesKeys, okvsTable1, nParties, type_okvs, secSemiHonest);

	std::vector <std::vector<block>> okvsTables(nParties - 3); //okvs of party 2 -> n-2
	for (u64 idxParty = 0; idxParty < okvsTables.size(); idxParty++)
		party2_encode(inputSets[idxParty + 1], aesKeys[idxParty], okvsTables[idxParty], type_okvs, secSemiHonest);

	std::vector<block> inputSet2PSI_n1; //party n-1
	partyn1_decode(inputSets[party_n1], aesKeys[aesKeys.size() - 1], okvsTables, inputSet2PSI_n1, type_okvs, secSemiHonest);

	std::vector<block> inputSet2PSI_n; //party n
	partyn_decode(inputSets[party_n], okvsTable1, inputSet2PSI_n, type_okvs, secSemiHonest);

	for (u64 i = 0; i < setSize; ++i)
	{
		if (i < intersection_size && inputSet2PSI_n1[i] != inputSet2PSI_n[i])
			std::cout << inputSet2PSI_n1[i] << " vs " << inputSet2PSI_n[i] << " \t expected = \n";

		if (i >= intersection_size && inputSet2PSI_n1[i] == inputSet2PSI_n[i])
			std::cout << inputSet2PSI_n1[i] << " vs " << inputSet2PSI_n[i] << " \t expected != \n";
	}
	std::cout << " ============== done ==============\n";

}

inline void partyO1(u64 myIdx, u64 nParties, u64 setSize, u64 type_okvs, u64 type_security)
{
	std::fstream textout;
	textout.open("./runtime_" + myIdx, textout.app | textout.out);

#pragma region setup
	u64  psiSecParam = 40, bitSize = 128, numChannelThreads = 1, okvsTableSize = setSize;
	u64 party_n1 = nParties - 2, party_n = nParties - 1; //party n-1 vs n
	double dataSent, Mbps, MbpsRecv, dataRecv;
	Timer timer;
	PRNG prng(_mm_set_epi32(4253465, 3434565, myIdx, myIdx));
	u64 expected_intersection = 3;// (*(u64*)&prng.get<block>()) % setSize;


	if (type_okvs == GbfOkvs)
		okvsTableSize = 60 * setSize;
	else if (type_okvs == PolyOkvs)
		okvsTableSize = setSize;


	std::string name("psi");
	BtIOService ios(0);
	std::vector<BtEndpoint> ep(nParties);
	std::vector<std::vector<Channel*>> chls(nParties);

	for (u64 i = 0; i < nParties; ++i)
	{
		if (i < myIdx)
		{
			u32 port = 1200 + i * 100 + myIdx;;//get the same port; i=1 & pIdx=2 =>port=102
			ep[i].start(ios, "localhost", port, false, name); //channel bwt i and pIdx, where i is sender
		}
		else if (i > myIdx)
		{
			u32 port = 1200 + myIdx * 100 + i;//get the same port; i=2 & pIdx=1 =>port=102
			ep[i].start(ios, "localhost", port, true, name); //channel bwt i and pIdx, where i is receiver
		}
	}


	for (u64 i = 0; i < nParties; ++i)
	{
		if (i != myIdx) {
			chls[i].resize(numChannelThreads);
			for (u64 j = 0; j < numChannelThreads; ++j)
				chls[i][j] = &ep[i].addChannel(name, name);
		}
	}

	u64 maskSize = roundUpTo(psiSecParam + 2 * std::log2(setSize) - 1, 8) / 8;


	PRNG prngSame(_mm_set_epi32(4253465, 3434565, 234435, 23987045)), prngDiff(_mm_set_epi32(434653, 23, myIdx, myIdx));
	std::vector<block> inputSet(setSize);

	for (u64 i = 0; i < expected_intersection; ++i)
		inputSet[i] = prngSame.get<block>();

	for (u64 i = expected_intersection; i < setSize; ++i)
		inputSet[i] = prngDiff.get<block>();
#pragma endregion

	u64 num_threads = nParties - 1; //for party 1

	timer.reset();
	auto start = timer.setTimePoint("start");



	std::vector<block> aesKeys(nParties - 2); //aesKeys[0] for party 2
	block aesReceivedKey; //aesKeys[0] for party 2

	std::vector<block> inputSet2PSI(setSize); //for party n-1 and n


	//====================================
	//============sending aes keys========
	if (myIdx == 0)
	{
		//generating aes keys
		for (u64 i = 0; i < aesKeys.size(); ++i)
			aesKeys[i] = prng.get<block>();

		for (u64 i = 1; i < nParties - 1; ++i)
			chls[i][0]->asyncSend(&aesKeys[i - 1], sizeof(block)); //sending aesKeys[0] to party 2

	/*	std::cout << IoStream::lock;
		for (u64 i = 0; i < aesKeys.size(); ++i)
			std::cout << aesKeys[i] << std::endl;
		std::cout << IoStream::unlock;*/
	}

	else if (myIdx < nParties - 1 && myIdx >0)
	{
		chls[0][0]->recv(&aesReceivedKey, sizeof(block));  //receiving aesKey from paty 1
	/*	std::cout << IoStream::lock;
		std::cout << aesReceivedKey <<  " - aesReceivedKey - " <<myIdx << std::endl;
		std::cout << IoStream::unlock;*/
	}

	//====================================
	//============compute encoding========


	/*std::cout << IoStream::lock;
	std::cout << inputSet[0] << " - inputSet - " << myIdx  << std::endl;
	std::cout << IoStream::unlock;*/

	if (myIdx == 0)
	{
		std::vector<block> okvsTable1; //okvs of party1
		party1_encode(inputSet, aesKeys, okvsTable1, nParties, type_okvs, type_security);
		chls[party_n][0]->send(okvsTable1.data(), okvsTable1.size() * sizeof(block)); //sending okvsTable1 to party n

	/*	std::cout << IoStream::lock;
		for (u64 i = 0; i < okvsTable1.size(); i++)
			std::cout << okvsTable1[i] << " - " << i << "okvsTable1 party1_encode - " << myIdx << " ->" << party_n << std::endl;
		std::cout << IoStream::unlock;*/

	}
	else if (myIdx < party_n1 && myIdx>0)
	{
		std::vector<block> okvsTable; //okvs of each party 2 -> n-2
		party2_encode(inputSet, aesReceivedKey, okvsTable, type_okvs, type_security);
		chls[party_n1][0]->send(okvsTable.data(), okvsTable.size() * sizeof(block)); //sending okvsTable to party n-1

		/*std::cout << IoStream::lock;
		for (u64 i = 0; i < 4; i++)
			std::cout << okvsTable[i] << " - okvsTable party2_encode: " << myIdx << " ->"<< party_n1 <<std::endl;
		std::cout << IoStream::unlock;*/

	}
	else if (myIdx == party_n1)
	{
		std::vector <std::vector<block>> okvsTables(nParties - 3); //okvs of party 2 -> n-2
		for (u64 idxParty = 0; idxParty < okvsTables.size(); idxParty++)
		{
			okvsTables[idxParty].resize(okvsTableSize);
			chls[idxParty + 1][0]->recv(okvsTables[idxParty].data(), okvsTables[idxParty].size() * sizeof(block)); //receving okvsTable from party 2->n-2

			/*std::cout << IoStream::lock;
			for (u64 i = 0; i < 4; i++)
				std::cout << okvsTables[idxParty][i] << " - okvsTable party2_encode - " << myIdx << " <-" << idxParty + 1 << std::endl;
			std::cout << IoStream::unlock;*/
		}

		partyn1_decode(inputSet, aesReceivedKey, okvsTables, inputSet2PSI, type_okvs, type_security);

		std::cout << IoStream::lock;
		for (u64 i = 0; i < 2; i++)
			std::cout << inputSet2PSI[i] << " - inputSet2PSI-n1 " << myIdx << std::endl;
		std::cout << IoStream::unlock;
	}
	else if (myIdx == party_n)
	{
		std::vector<block> okvsTable;
		okvsTable.resize(okvsTableSize);
		chls[0][0]->recv(okvsTable.data(), okvsTable.size() * sizeof(block)); //receving okvsTable from party 1

		/*std::cout << IoStream::lock;
		for (u64 i = 0; i < okvsTable.size(); i++)
			std::cout << okvsTable[i] << " - " << i << "okvsTable party1_encode: " << myIdx << " <-" << 0 << std::endl;
		std::cout << IoStream::unlock;*/

		partyn_decode(inputSet, okvsTable, inputSet2PSI, type_okvs, type_security);

		std::cout << IoStream::lock;
		for (u64 i = 0; i < 2; i++)
			std::cout << inputSet2PSI[i] << " - inputSet2PSI-n " << myIdx << std::endl;
		std::cout << IoStream::unlock;
	}

	//====================================
	//============compute 2psi========

	if (myIdx == party_n1 || myIdx == party_n) //for 2psi
	{
		std::vector<KkrtNcoOtReceiver> otRecv(2);
		std::vector<KkrtNcoOtSender> otSend(2);
		OPPRFSender send;
		OPPRFReceiver recv;
		binSet bins;

		bins.init(myIdx, 2, setSize, psiSecParam, 1, 1);
		//	bins.mMaskSize = 8;
		u64 otCountSend = bins.mSimpleBins.mBins.size();
		u64 otCountRecv = bins.mCuckooBins.mBins.size();

		if (myIdx == party_n1) {
			//I am a sender -> party_n
			send.init(bins.mOpt, 2, setSize, psiSecParam, bitSize, chls[party_n], otCountSend, otSend[1], otRecv[1], prng.get<block>(), false);

		}
		else if (myIdx == party_n) {
			//I am a recv <-party_n1
			recv.init(bins.mOpt, 2, setSize, psiSecParam, bitSize, chls[party_n1], otCountRecv, otRecv[0], otSend[0], ZeroBlock, false);
		}


		//##########################
		//### Hashing
		//##########################
		bins.hashing2Bins(inputSet2PSI, 1);


		if (myIdx == party_n1) {
			//I am a sender to my next neigbour
			send.getOPRFkeysfor2PSI(1, bins, chls[party_n], false);
			send.sendLastPSIMessage(1, bins, chls[party_n]);

		}
		else if (myIdx == party_n) {
			//I am a recv to my previous neigbour
			recv.getOPRFkeysfor2PSI(0, bins, chls[party_n1], false);
			recv.compute2PSI(0, bins, chls[party_n1]);
		}






		//std::cout << IoStream::lock;
		//if (myIdx == 0) //sender
		//{
		//	for (int i = 0; i < bins.mSimpleBins.mBins[0].mValOPRF[1].size(); i++)
		//	{

		//		std::cout << bins.mSimpleBins.mBins[0].mValOPRF[1][i] << std::endl;
		//		//Log::out << recvPayLoads[2][i] << Log::endl;
		//	}
		//	std::cout << "------------" << std::endl;
		//}
		//if (myIdx == 1)
		//{
		//	std::cout << bins.mCuckooBins.mBins[0].mValOPRF[0] << std::endl;

		//	//for (int i = 0; i < bins.mCuckooBins.mBins[0].mValOPRF[0].size(); i++)
		//	//{
		//	//	//Log::out << recvPayLoads[i] << Log::endl;
		//	//	std::cout << sendPayLoads[i] << std::endl;
		//	//}
		//}
		//std::cout << IoStream::unlock;


		//##########################
		//### online phasing - compute intersection
		//##########################

		if (myIdx == party_n) {
			Log::out << "mIntersection.size(): " << recv.mIntersection.size() << Log::endl;
			for (u64 i = 0; i < recv.mIntersection.size(); ++i)
			{
				std::cout << recv.mIntersection[i] << " - " << inputSet[recv.mIntersection[i]] << std::endl;

			}
		}
	}


	//close chanels 
	for (u64 i = 0; i < nParties; ++i)
		if (i != myIdx)
			for (u64 j = 0; j < numChannelThreads; ++j)
				chls[i][j]->close();

	for (u64 i = 0; i < nParties; ++i)
		if (i != myIdx)
			ep[i].stop();

	ios.stop();
}
