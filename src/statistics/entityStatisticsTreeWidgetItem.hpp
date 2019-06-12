/*
* Copyright (C) 2017-2019, Emilien Vallot, Christophe Calmejane and other contributors

* This file is part of Hive.

* Hive is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.

* Hive is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public License
* along with Hive.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <la/avdecc/controller/internals/avdeccControlledEntity.hpp>

#include "avdecc/helper.hpp"
#include "avdecc/controllerManager.hpp"

#include <chrono>

#include <QObject>
#include <QTreeWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

class EntityStatisticsTreeWidgetItem : public QObject, public QTreeWidgetItem
{
public:
	EntityStatisticsTreeWidgetItem(la::avdecc::UniqueIdentifier const entityID, std::uint64_t const aecpRetryCounter, std::uint64_t const aecpTimeoutCounter, std::uint64_t const aecpUnexpectedResponseCounter, std::chrono::milliseconds const& aecpResponseAverageTime, std::uint64_t const aemAecpUnsolicitedCounter, std::chrono::milliseconds const& enumerationTime, QTreeWidget* parent = nullptr);
	virtual ~EntityStatisticsTreeWidgetItem() override
	{
		takeChildren();
	}

private:
	void updateAecpRetryCounter(std::uint64_t const value) noexcept;
	void updateAecpTimeoutCounter(std::uint64_t const value) noexcept;
	void updateAecpUnexpectedResponseCounter(std::uint64_t const value) noexcept;
	void updateAecpResponseAverageTime(std::chrono::milliseconds const& value) noexcept;
	void updateAemAecpUnsolicitedCounter(std::uint64_t const value) noexcept;

	la::avdecc::UniqueIdentifier const _entityID{};

	// Statistics
	QTreeWidgetItem _aecpRetryCounterItem{ this };
	QTreeWidgetItem _aecpTimeoutCounterItem{ this };
	QTreeWidgetItem _aecpUnexpectedResponseCounterItem{ this };
	QTreeWidgetItem _aecpResponseAverageTimeItem{ this };
	QTreeWidgetItem _aemAecpUnsolicitedCounterItem{ this };
	QTreeWidgetItem _enumerationTimeItem{ this };
};