#include "EmulationStation.h"
#include "guis/GuiMenu.h"
#include "guis/GuiEmulatorList.h"
#include "guis/GuiWifi.h"
#include "guis/GuiSystemSettings.h"
#include "Window.h"
#include "Sound.h"
#include "Log.h"
#include "Settings.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiScraperStart.h"
#include "guis/GuiDetectDevice.h"
#include "views/ViewController.h"

#include <Eigen/Dense>

#include "components/ButtonComponent.h"
#include "components/SwitchComponent.h"
#include "components/SliderComponent.h"
#include "components/TextComponent.h"
#include "components/OptionListComponent.h"
#include "components/MenuComponent.h"
#include "components/ProgressBarComponent.h"
#include "VolumeControl.h"
#include "scrapers/GamesDBScraper.h"
#include "scrapers/TheArchiveScraper.h"

GuiMenu::GuiMenu(Window* window) : GuiComponent(window), mMenu(window, "MAIN MENU"), mVersion(window), mNetInfo(window)
{
	// MAIN MENU
	// CONFIGURE INPUT >
	// APPS >
	// EMULATORS >
	// SYSTEM >
	// QUIT >

	// [version]

	addEntry("CONFIGURE INPUT", 0x777777FF, true, 
		[this] {
			Window* window = mWindow;
			window->pushGui(new GuiMsgBox(window, "ARE YOU SURE YOU WANT TO CONFIGURE INPUT?", "YES",
				[window] {
					window->pushGui(new GuiDetectDevice(window, false, nullptr));
				}, "NO", nullptr)
			);
	});

	addEntry("APPS", 0x777777FF, true,
		[this] {
			auto s = new GuiSettings(mWindow, "APPS");
			
			Window* window = mWindow;

			ComponentListRow row;
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY START ARMBIAN DESKTOP?", "YES", 
				[] { 
					system("export LD_LIBRARY_PATH=/usr/local && startx > /dev/null 2>&1");
					
				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "DESKTOP", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "ARE YOU SURE YOU WANT TO LAUNCH OPENELEC?", "YES", 
				[] { 
					system("sudo mkimage -C none -A arm -T script -d /boot/boot.kodi.cmd /boot/boot.scr >/dev/null 2>&1 && sudo reboot >/dev/null 2>&1");

				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "OPENELEC", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			mWindow->pushGui(s);
	});

	addEntry("EMULATORS", 0x777777FF, true, [this, window] {
		mWindow->pushGui(new GuiEmulatorList(window));
	});

	addEntry("SYSTEM", 0x777777FF, true, [this] {
		mWindow->pushGui(new GuiSystemSettings(mWindow));
	});
		auto openScrapeNow = [this] { mWindow->pushGui(new GuiScraperStart(mWindow)); };
			addEntry("SCRAPER", 0x777777FF, true, 
			[this, openScrapeNow] { 
			auto s = new GuiSettings(mWindow, "SCRAPER");

			// scrape from
			auto scraper_list = std::make_shared< OptionListComponent< std::string > >(mWindow, "SCRAPE FROM", false);
			std::vector<std::string> scrapers = getScraperList();
			for(auto it = scrapers.begin(); it != scrapers.end(); it++)
				scraper_list->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));

			s->addWithLabel("SCRAPE FROM", scraper_list);
			s->addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper", scraper_list->getSelected()); });

			// scrape ratings
			auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
			scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
			s->addWithLabel("SCRAPE RATINGS", scrape_ratings);
			s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState()); });

			// scrape now
			ComponentListRow row;
			std::function<void()> openAndSave = openScrapeNow;
			openAndSave = [s, openAndSave] { s->save(); openAndSave(); };
			row.makeAcceptInputHandler(openAndSave);

			auto scrape_now = std::make_shared<TextComponent>(mWindow, "SCRAPE NOW", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
			auto bracket = makeArrow(mWindow);
			row.addElement(scrape_now, true);
			row.addElement(bracket, false);
			s->addRow(row);

			mWindow->pushGui(s);
	});

	addEntry("QUIT", 0x777777FF, true, 
		[this] {
			auto s = new GuiSettings(mWindow, "QUIT");
			
			Window* window = mWindow;

			ComponentListRow row;
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES",
				[] {
					if(quitES("/tmp/es-restart") != 0)
						LOG(LogWarning) << "Restart terminated with non-zero result!";
				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "RESTART EMULATIONSTATION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY RESTART?", "YES", 
				[] { 
					if(quitES("/tmp/es-sysrestart") != 0)
						LOG(LogWarning) << "Restart terminated with non-zero result!";
				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "RESTART SYSTEM", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			row.elements.clear();
			row.makeAcceptInputHandler([window] {
				window->pushGui(new GuiMsgBox(window, "REALLY SHUTDOWN?", "YES", 
				[] { 
					if (quitES("/tmp/es-shutdown") != 0)
					{
						LOG(LogWarning) << "Shutdown terminated with non-zero result!";
					}
				}, "NO", nullptr));
			});
			row.addElement(std::make_shared<TextComponent>(window, "SHUTDOWN SYSTEM", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
			s->addRow(row);

			if(Settings::getInstance()->getBool("ShowExit"))
			{
				row.elements.clear();
				row.makeAcceptInputHandler([window] {
					window->pushGui(new GuiMsgBox(window, "REALLY QUIT?", "YES", 
					[] { 
						SDL_Event ev;
						ev.type = SDL_QUIT;
						SDL_PushEvent(&ev);
					}, "NO", nullptr));
				});
				row.addElement(std::make_shared<TextComponent>(window, "QUIT EMULATIONSTATION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
				s->addRow(row);
			}

			mWindow->pushGui(s);
	});
	

	mVersion.setFont(Font::get(FONT_SIZE_SMALL));
	mVersion.setColor(0xAAAAFFFF);
	mVersion.setText("EMULATIONSTATION V" + strToUpper(PROGRAM_VERSION_STRING));
	mVersion.setAlignment(ALIGN_CENTER);

	addChild(&mMenu);
	addChild(&mVersion);
	addChild(&mNetInfo);


	setSize(mMenu.getSize());
	setPosition((Renderer::getScreenWidth() - mSize.x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiMenu::onSizeChanged()
{
	mVersion.setSize(mSize.x(), 0);
	mVersion.setPosition(0, mSize.y() - mVersion.getSize().y());
}

void GuiMenu::addEntry(const char* name, unsigned int color, bool add_arrow, const std::function<void()>& func)
{
	std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM);
	
	// populate the list
	ComponentListRow row;
	row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

	if(add_arrow)
	{
		std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
		row.addElement(bracket, false);
	}
	
	row.makeAcceptInputHandler(func);

	mMenu.addRow(row);
}

bool GuiMenu::input(InputConfig* config, Input input)
{
	if(GuiComponent::input(config, input))
		return true;

	if((config->isMappedTo("b", input) || config->isMappedTo("start", input)) && input.value != 0)
	{
		delete this;
		return true;
	}

	return false;
}

void GuiMenu::update(int deltatime) {
	mNetInfo.update(deltatime);
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts;
	prompts.push_back(HelpPrompt("up/down", "choose"));
	prompts.push_back(HelpPrompt("a", "select"));
	prompts.push_back(HelpPrompt("start", "close"));
	return prompts;
}
