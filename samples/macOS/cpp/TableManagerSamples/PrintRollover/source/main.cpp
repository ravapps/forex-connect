#include "stdafx.h"
#include <iostream>
#include "SessionStatusListener.h"
#include "LoginParams.h"
#include "SampleParams.h"
#include "CommonSources.h"
#include <IO2GRolloverProvider.h>

void printHelp(std::string &);
bool checkObligatoryParams(LoginParams *, SampleParams *);
void printSampleParams(std::string &, LoginParams *, SampleParams *);
void printRollover(IO2GRolloverProvider *, IO2GAccountRow *, IO2GOfferRow *);


int main(int argc, char *argv[])
{
	std::string procName = "PrintRollover";
	if (argc == 1)
	{
		printHelp(procName);
		return -1;
	}

	LoginParams *loginParams = new LoginParams(argc, argv);
	SampleParams *sampleParams = new SampleParams(argc, argv);

	printSampleParams(procName, loginParams, sampleParams);
	if (!checkObligatoryParams(loginParams, sampleParams))
	{
		delete loginParams;
		delete sampleParams;
		return -1;
	}

	IO2GSession *session = CO2GTransport::createSession();
	session->useTableManager(Yes, 0);

	SessionStatusListener *sessionListener = new SessionStatusListener(session, false,
		loginParams->getSessionID(),
		loginParams->getPin());
	session->subscribeSessionStatus(sessionListener);

	bool bConnected = login(session, sessionListener, loginParams);
	bool bWasError = false;

	if (bConnected)
	{
		bool bIsAccountEmpty = !sampleParams->getAccount() || strlen(sampleParams->getAccount()) == 0;

		O2G2Ptr<IO2GTableManager> tableManager = session->getTableManager();
		O2GTableManagerStatus managerStatus = tableManager->getStatus();
		while (managerStatus == TablesLoading)
		{
			Sleep(50);
			managerStatus = tableManager->getStatus();
		}

		if (managerStatus == TablesLoadFailed)
		{
			std::cout << "Cannot refresh all tables of table manager" << std::endl;
		}

		O2G2Ptr<IO2GAccountRow> account = getAccount(tableManager, sampleParams->getAccount());

		if (account && strcmp(account->getMaintenanceType(), "0") != 0) // not netting account
		{
			if (bIsAccountEmpty)
			{
				sampleParams->setAccount(account->getAccountID());
				std::cout << "Account: " << sampleParams->getAccount() << std::endl;
			}
			O2G2Ptr<IO2GOfferRow> offer = getOffer(tableManager, sampleParams->getInstrument());
			if (offer)
			{
				IO2GRolloverProvider *rolloverProvider = session->getRolloverProvider();
				O2GRolloverStatus rolloverStatus = rolloverProvider->getStatus();
				printRollover(rolloverProvider, account, offer);
			}
			else
			{
				std::cout << "The instrument '" << sampleParams->getInstrument() << "' is not valid" << std::endl;
				bWasError = true;
			}
		}
		else
		{
			std::cout << "No valid accounts" << std::endl;
			bWasError = true;
		}
	}
	else 
	{
		bWasError = true;
	}

	logout(session, sessionListener);

	session->unsubscribeSessionStatus(sessionListener);
	sessionListener->release();
	session->release();

	delete loginParams;
	delete sampleParams;

	if (bWasError)
		return -1;
	return 0;
}

void printRollover(IO2GRolloverProvider *rolloverProvider, IO2GAccountRow *account, IO2GOfferRow *offer)
{
	std::string discription;
	double rolloverBuy;
	double rolloverSell;

	rolloverBuy = rolloverProvider->getRolloverBuy(offer, account);
	rolloverSell = rolloverProvider->getRolloverSell(offer, account);

	std::cout << discription <<"Rollover: " << rolloverBuy << " (buy), " << rolloverSell << " (sell)" << std::endl;
}

void printSampleParams(std::string &sProcName, LoginParams *loginParams, SampleParams *sampleParams)
{
    std::cout << "Running " << sProcName << " with arguments:" << std::endl;

    // Login (common) information
    if (loginParams)
    {
        std::cout << loginParams->getLogin() << " * "
                  << loginParams->getURL() << " "
                  << loginParams->getConnection() << " "
                  << loginParams->getSessionID() << " "
                  << loginParams->getPin() << std::endl;
    }

    // Sample specific information
    if (sampleParams)
    {
        std::cout << "Instrument='" << sampleParams->getInstrument() << "', "
                << "Account='" << sampleParams->getAccount() << "'"
                << std::endl;
    }
}

void printHelp(std::string &sProcName)
{
    std::cout << sProcName << " sample parameters:" << std::endl << std::endl;
            
    std::cout << "/login | --login | /l | -l" << std::endl;
    std::cout << "Your user name." << std::endl << std::endl;
                
    std::cout << "/password | --password | /p | -p" << std::endl;
    std::cout << "Your password." << std::endl << std::endl;
                
    std::cout << "/url | --url | /u | -u" << std::endl;
    std::cout << "The server URL. For example, http://www.fxcorporate.com/Hosts.jsp." << std::endl << std::endl;
                
    std::cout << "/connection | --connection | /c | -c" << std::endl;
    std::cout << "The connection name. For example, \"Demo\" or \"Real\"." << std::endl << std::endl;
                
    std::cout << "/sessionid | --sessionid " << std::endl;
    std::cout << "The database name. Required only for users who have accounts in more than one database. Optional parameter." << std::endl << std::endl;
                
    std::cout << "/pin | --pin " << std::endl;
    std::cout << "Your pin code. Required only for users who have a pin. Optional parameter." << std::endl << std::endl;
                
    std::cout << "/instrument | --instrument | /i | -i" << std::endl;
    std::cout << "An instrument which you want to use in sample. For example, \"EUR/USD\"." << std::endl << std::endl;
            
    std::cout << "/account | --account " << std::endl;
    std::cout << "An account which you want to use in sample. Optional parameter." << std::endl << std::endl;
}

bool checkObligatoryParams(LoginParams *loginParams, SampleParams *sampleParams)
{
    /* Check login parameters. */
    if (strlen(loginParams->getLogin()) == 0)
    {
        std::cout << LoginParams::Strings::loginNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getPassword()) == 0)
    {
        std::cout << LoginParams::Strings::passwordNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getURL()) == 0)
    {
        std::cout << LoginParams::Strings::urlNotSpecified << std::endl;
        return false;
    }
    if (strlen(loginParams->getConnection()) == 0)
    {
        std::cout << LoginParams::Strings::connectionNotSpecified << std::endl;
        return false;
    }

    /* Check other parameters. */
    if (strlen(sampleParams->getInstrument()) == 0)
    {
        std::cout << SampleParams::Strings::instrumentNotSpecified << std::endl;
        return false;
    }

    return true;
}

