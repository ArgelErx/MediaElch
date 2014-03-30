/*
    Copyright 2012-2014 Daniel Kabel.
    Copyright 2014 Udo Schläpfer.

    This file is part of MediaElch.

    MediaElch is free software: you can redistribute it and/or modify it under the terms of
    the GNU Lesser General Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    MediaElch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
    without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See
    the GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License along with MediaElch.
    If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef MOVIESEARCHWIDGET_H
#define MOVIESEARCHWIDGET_H


#include <QWidget>
#include <QTableWidgetItem>

#include "data/ScraperInterface.h"
#include "globals/Globals.h"


namespace Ui
{
    class MovieSearchWidget;
}


class MovieSearchWidget : public QWidget
{
    Q_OBJECT

    public:

        explicit MovieSearchWidget( QWidget* parent = 0 );

        ~MovieSearchWidget();

    public slots:

        QString scraperId();

        QString scraperMovieId();

        QList<int> infosToLoad();

        QMap<ScraperInterface*, QString> customScraperIds();

        void search( QString searchString, QString id, QString tmdbId );

    private slots:

        void search();

        void showResults( QList<ScraperSearchResult> results );

        //! \brief Handle single click on item in result table.
        void onMovieClicked( QTableWidgetItem* item );

        //! \brief Update the overview after a item in the result table has been selected.
        void onUpdateOverview( QList<ScraperSearchResult> results );

        //! \brief Update remote database id after switch of scraper.
        void onUpdateSearchStringId();

        //! \brief Handle switch of search identifier.
        void onSearchIdentifierSwitched();

        void chkToggled();

        void chkAllToggled( bool toggled );

    private:

        void clear();

        void setChkBoxesEnabled( QList<int> scraperSupports );

        void setupScrapers();

        //! \brief Switch between seach by id (\a flag == true) and search by movie title (\a flag == false).
        void searchById( bool flag );

        //! \brief Retrive the selected scraper.
        ScraperInterface* getCurrentScraper();

    private:

        Ui::MovieSearchWidget* ui;

        QString m_scraperId;

        QString m_scraperMovieId;

        QList<int> m_infosToLoad;

        QMap<ScraperInterface*, QString> m_customScraperIds;

        ScraperInterface* m_currentCustomScraper;

        QString m_id;

        QString m_tmdbId;

}; // class MovieSearchWidget


#endif // MOVIESEARCHWIDGET_H
