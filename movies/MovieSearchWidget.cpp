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


#include <QDebug>


#include "MovieSearchWidget.h"
#include "ui_MovieSearchWidget.h"
#include "globals/Manager.h"
#include "scrapers/CustomMovieScraper.h"


enum CustomUserRole
{
    UserRoleId = Qt::UserRole,
    UserRoleOverview
};



MovieSearchWidget::MovieSearchWidget( QWidget* parent )
    : QWidget( parent ),
      ui( new Ui::MovieSearchWidget ),
      m_scraperId(),
      m_scraperMovieId(),
      m_infosToLoad(),
      m_customScraperIds(),
      m_currentCustomScraper( 0 ),
      m_id(),
      m_tmdbId()
{
    qDebug() << Q_FUNC_INFO;

    this->ui->setupUi( this );
    this->ui->results->verticalHeader()->setSectionResizeMode( QHeaderView::ResizeToContents );
    this->ui->searchStringId->setType( MyLineEdit::TypeLoading );
    this->ui->searchStringTitle->setType( MyLineEdit::TypeLoading );

    foreach( ScraperInterface* scraper, Manager::instance()->scrapers() )
    {
        connect( scraper, SIGNAL( searchDone( QList<ScraperSearchResult> ) ), this, SLOT( showResults( QList<ScraperSearchResult> ) ) );
    }
    setupScrapers();

    connect( this->ui->comboScraper,      SIGNAL( currentIndexChanged( int ) ),       this, SLOT( onUpdateSearchStringId() ), Qt::QueuedConnection );
    connect( this->ui->comboScraper,      SIGNAL( currentIndexChanged( int ) ),       this, SLOT( search() ), Qt::QueuedConnection );
    connect( this->ui->rBtnSearchId,      SIGNAL( clicked() ),                        this, SLOT( onSearchIdentifierSwitched() ) );
    connect( this->ui->searchStringId,    SIGNAL( returnPressed() ),                  this, SLOT( search() ) );
    connect( this->ui->rBtnSearchTitle,   SIGNAL( clicked() ),                        this, SLOT( onSearchIdentifierSwitched() ) );
    connect( this->ui->searchStringTitle, SIGNAL( returnPressed() ),                  this, SLOT( search() ) );
    connect( this->ui->results,           SIGNAL( itemClicked( QTableWidgetItem* ) ), this, SLOT( onMovieClicked( QTableWidgetItem* ) ) );

    this->ui->chkActors->setMyData( MovieScraperInfos::Actors );
    this->ui->chkBackdrop->setMyData( MovieScraperInfos::Backdrop );
    this->ui->chkCertification->setMyData( MovieScraperInfos::Certification );
    this->ui->chkCountries->setMyData( MovieScraperInfos::Countries );
    this->ui->chkDirector->setMyData( MovieScraperInfos::Director );
    this->ui->chkGenres->setMyData( MovieScraperInfos::Genres );
    this->ui->chkOverview->setMyData( MovieScraperInfos::Overview );
    this->ui->chkPoster->setMyData( MovieScraperInfos::Poster );
    this->ui->chkRating->setMyData( MovieScraperInfos::Rating );
    this->ui->chkReleased->setMyData( MovieScraperInfos::Released );
    this->ui->chkRuntime->setMyData( MovieScraperInfos::Runtime );
    this->ui->chkSet->setMyData( MovieScraperInfos::Set );
    this->ui->chkStudios->setMyData( MovieScraperInfos::Studios );
    this->ui->chkTagline->setMyData( MovieScraperInfos::Tagline );
    this->ui->chkTitle->setMyData( MovieScraperInfos::Title );
    this->ui->chkTrailer->setMyData( MovieScraperInfos::Trailer );
    this->ui->chkWriter->setMyData( MovieScraperInfos::Writer );
    this->ui->chkLogo->setMyData( MovieScraperInfos::Logo );
    this->ui->chkClearArt->setMyData( MovieScraperInfos::ClearArt );
    this->ui->chkCdArt->setMyData( MovieScraperInfos::CdArt );
    this->ui->chkBanner->setMyData( MovieScraperInfos::Banner );
    this->ui->chkThumb->setMyData( MovieScraperInfos::Thumb );
    this->ui->chkTags->setMyData( MovieScraperInfos::Tags );

    foreach( MyCheckBox* box, this->ui->groupBox->findChildren<MyCheckBox*>() )
    {
        if( box->myData().toInt() > 0 )
        {
            connect( box, SIGNAL( clicked() ), this, SLOT( chkToggled() ) );
        }
    }
    connect( this->ui->chkUnCheckAll, SIGNAL( clicked( bool ) ), this, SLOT( chkAllToggled( bool ) ) );
}


MovieSearchWidget::~MovieSearchWidget()
{
    qDebug() << Q_FUNC_INFO;

    delete this->ui;
}


void MovieSearchWidget::clear()
{
    qDebug() << Q_FUNC_INFO;

    this->ui->results->clearContents();
    this->ui->results->setRowCount( 0 );
}


void MovieSearchWidget::setupScrapers()
{
    qDebug() << Q_FUNC_INFO;

    this->ui->comboScraper->blockSignals( true );
    this->ui->comboScraper->clear();

    foreach( ScraperInterface* scraper, Manager::instance()->scrapers() )
    {
        if( ( ! Settings::instance()->showAdultScrapers() ) && scraper->isAdult() )
        {
            continue;
        }

        if( scraper->isAdult() )
        {
            this->ui->comboScraper->addItem( QIcon( ":/img/heart_red_open.png" ), scraper->name(), scraper->identifier() );
        }
        else
        {
            this->ui->comboScraper->addItem( scraper->name(), scraper->identifier() );
        }
    }

    this->ui->comboScraper->setCurrentIndex( 0 );
    if( Settings::instance()->currentMovieScraper() < this->ui->comboScraper->count() )
    {
        this->ui->comboScraper->setCurrentIndex( Settings::instance()->currentMovieScraper() );
    }

    ScraperInterface* currentScraper = this->getCurrentScraper();
    if( currentScraper->identifier() == "tmdb" )
    {
        this->ui->lblOverview->show();
        this->ui->teOverview->show();
    }
    else
    {
        this->ui->lblOverview->hide();
        this->ui->teOverview->hide();
    }

    this->ui->comboScraper->blockSignals( false );
}


void MovieSearchWidget::search( QString searchString, QString id, QString tmdbId )
{
    qDebug() << Q_FUNC_INFO;

    QString searchStringTitle = searchString.replace( ".", " " );
    searchStringTitle = searchStringTitle.replace( "_", " " );
    m_id = id;
    m_tmdbId = tmdbId;
    m_currentCustomScraper = 0;
    m_customScraperIds.clear();

    this->setupScrapers();
    this->ui->comboScraper->setEnabled( true );
    this->ui->groupBox->setEnabled( true );
    this->ui->searchStringTitle->setText( searchStringTitle );
    this->onUpdateSearchStringId();

    search();
}


void MovieSearchWidget::search()
{
    qDebug() << Q_FUNC_INFO;

    ScraperInterface *scraper = this->getCurrentScraper();
    if( ! scraper )
    {
        return;
    }

    if (m_scraperId == "custom-movie")
    {
        this->m_currentCustomScraper = CustomMovieScraper::instance()->titleScraper();
    }

    this->setChkBoxesEnabled( Manager::instance()->scraper( m_scraperId )->scraperSupports() );
    this->clear();
    this->ui->comboScraper->setEnabled( false );
    this->ui->rBtnSearchId->setEnabled( false );
    this->ui->rBtnSearchTitle->setEnabled( false );

    if( this->ui->rBtnSearchId->isChecked() )
    {
        this->ui->searchStringId->setLoading( true );
        scraper->search( this->ui->searchStringId->text() );
    }
    else
    {
        this->ui->searchStringTitle->setLoading( true );
        scraper->search( this->ui->searchStringTitle->text() );
    }

    Settings::instance()->setCurrentMovieScraper( this->ui->comboScraper->currentIndex() );
}


void MovieSearchWidget::showResults( QList<ScraperSearchResult> results )
{
    qDebug() << Q_FUNC_INFO << ": results.count():" << results.count();

    this->ui->comboScraper->setEnabled( m_customScraperIds.isEmpty() );

    if( this->ui->rBtnSearchId->isChecked() )
    {
        this->ui->searchStringId->setLoading( false );
    }
    else
    {
        this->ui->searchStringTitle->setLoading( false );
    }
    this->ui->rBtnSearchId->setEnabled( true );
    this->ui->rBtnSearchTitle->setEnabled( true );

    foreach( const ScraperSearchResult& result, results )
    {
        QTableWidgetItem* item = 0;

        if( ! result.released.isNull() )
        {
            item = new QTableWidgetItem( QString( "%1 (%2)" ).arg( result.name )
                                                             .arg( result.released.toString( "yyyy" ) ) );
        }
        else
        {
            item = new QTableWidgetItem( QString( "%1" ).arg( result.name ) );
        }

        if( ! ( result.overview.isNull() || result.overview.isEmpty() ) )
        {
            item->setData( UserRoleOverview, result.overview );
        }

        item->setData( UserRoleId, result.id );

        int row = this->ui->results->rowCount();
        this->ui->results->insertRow( row );
        this->ui->results->setItem( row, 0, item );
    }
}


void MovieSearchWidget::onMovieClicked( QTableWidgetItem* item )
{
    qDebug() << Q_FUNC_INFO << ": item->text():" << item->text();

    if( ( this->m_scraperId == "custom-movie" ) || ( ! this->m_customScraperIds.isEmpty() ) )
    {
        this->ui->comboScraper->setEnabled( false );
        this->ui->groupBox->setEnabled( false );

        if( this->m_currentCustomScraper == CustomMovieScraper::instance()->titleScraper() )
        {
            this->m_customScraperIds.clear();
        }

        this->m_customScraperIds.insert( this->m_currentCustomScraper,
                                         item->data( UserRoleId ).toString() );
        QList<ScraperInterface*> scrapers = CustomMovieScraper::instance()->scrapersNeedSearch( infosToLoad(),
                                                                                                this->m_customScraperIds );
        if( scrapers.isEmpty() )
        {
            m_scraperId = "custom-movie";
        }
        else
        {
            this->m_currentCustomScraper = scrapers.first();
            for( int i = 0, n = this->ui->comboScraper->count() ; i < n ; ++i )
            {
                if( this->ui->comboScraper->itemData( i, UserRoleId ).toString() == this->m_currentCustomScraper->identifier() )
                {
                    this->ui->comboScraper->setCurrentIndex( i );
                    break;
                }
            }
        }
    }
    else
    {
        this->m_scraperMovieId = item->data( UserRoleId ).toString();
        this->m_customScraperIds.clear();
    }

    if( ! this->ui->results->currentItem()->data( UserRoleOverview ).isNull() )
    {
        // Use cached overview.
        this->ui->teOverview->setPlainText( this->ui->results->currentItem()->data( UserRoleOverview ).toString() );
        return;
    }

    ScraperInterface *scraper = this->getCurrentScraper();
    if( scraper && ( scraper->identifier() == "tmdb" ) )
    {
        this->ui->comboScraper->setEnabled( false );
        this->ui->searchStringId->setEnabled( false );
        this->ui->searchStringTitle->setEnabled( false );
        this->ui->results->setEnabled( false );

        // Reroute scraper to update plot for selected movie.
        disconnect( scraper, 0, this, 0 );
        connect( scraper, SIGNAL( searchDone( QList<ScraperSearchResult> ) ),
                 this,      SLOT( onUpdateOverview( QList<ScraperSearchResult> ) ) );

        // Search movie by id, retriving the plot.
        scraper->search( "id" + item->data( UserRoleId ).toString() );
    }
}


void MovieSearchWidget::onUpdateOverview( QList<ScraperSearchResult> results )
{
    qDebug() << Q_FUNC_INFO << ": results.count(): " << results.count();

    if( results.count() == 1 )
    {
        // This is the result of an id search, there should never be more than one result.
        ScraperSearchResult &result = results.first();
        this->ui->results->currentItem()->setData( UserRoleOverview, result.overview );
        this->ui->teOverview->setPlainText( this->ui->results->currentItem()->data( UserRoleOverview ).toString() );
    }
    else
    {
        this->ui->results->currentItem()->setData( UserRoleOverview, "" );
        this->ui->teOverview->setPlainText( "" );
    }

    ScraperInterface *scraper = getCurrentScraper();
    if( scraper )
    {
        // Reroute scraper to present search results.
        disconnect( scraper, 0, this, 0 );
        connect( scraper, SIGNAL( searchDone( QList<ScraperSearchResult>) ),
                 this,      SLOT( showResults( QList<ScraperSearchResult> ) ) );
    }

    this->ui->comboScraper->setEnabled( this->m_customScraperIds.isEmpty() );
    this->searchById( this->ui->rBtnSearchId->isChecked() );
    this->ui->results->setEnabled( true );
}


void MovieSearchWidget::chkToggled()
{
    qDebug() << Q_FUNC_INFO;

    this->m_infosToLoad.clear();
    bool allToggled = true;

    foreach( MyCheckBox* box, this->ui->groupBox->findChildren<MyCheckBox*>() )
    {
        if( box->isChecked() && ( box->myData().toInt() > 0 ) )
        {
            this->m_infosToLoad.append( box->myData().toInt() );
        }
        if( ( ! box->isChecked() ) && ( box->myData().toInt() > 0 ) && box->isEnabled() )
        {
            allToggled = false;
        }
    }
    this->ui->chkUnCheckAll->setChecked( allToggled );

    QString scraperId = this->ui->comboScraper->itemData( this->ui->comboScraper->currentIndex(), UserRoleId ).toString();
    Settings::instance()->setScraperInfos( WidgetMovies, scraperId, this->m_infosToLoad );
}


void MovieSearchWidget::chkAllToggled( bool toggled )
{
    qDebug() << Q_FUNC_INFO << ": toggled: " << toggled;

    foreach( MyCheckBox* box, this->ui->groupBox->findChildren<MyCheckBox*>() )
    {
        if( ( box->myData().toInt() > 0 ) && box->isEnabled() )
        {
            box->setChecked( toggled );
        }
    }

    this->chkToggled();
}


QString MovieSearchWidget::scraperId()
{
    qDebug() << Q_FUNC_INFO;

    return this->m_scraperId;
}


QString MovieSearchWidget::scraperMovieId()
{
    qDebug() << Q_FUNC_INFO;

    return this->m_scraperMovieId;
}


QList<int> MovieSearchWidget::infosToLoad()
{
    qDebug() << Q_FUNC_INFO;

    return this->m_infosToLoad;
}


void MovieSearchWidget::setChkBoxesEnabled( QList<int> scraperSupports )
{
    qDebug() << Q_FUNC_INFO;

    QString scraperId = this->ui->comboScraper->itemData( this->ui->comboScraper->currentIndex(), UserRoleId ).toString();
    QList<int> infos = Settings::instance()->scraperInfos( WidgetMovies, scraperId );

    foreach( MyCheckBox* box, this->ui->groupBox->findChildren<MyCheckBox*>() )
    {
        box->setEnabled( scraperSupports.contains( box->myData().toInt() ) );
        box->setChecked(    ( infos.contains( box->myData().toInt() ) || infos.isEmpty() )
                         && scraperSupports.contains( box->myData().toInt() ) );
    }

    this->chkToggled();
}


QMap<ScraperInterface*, QString> MovieSearchWidget::customScraperIds()
{
    qDebug() << Q_FUNC_INFO;

    return this->m_customScraperIds;
}


void MovieSearchWidget::onUpdateSearchStringId()
{
    qDebug() << Q_FUNC_INFO;

    ScraperInterface *scraper = getCurrentScraper();
    if( ! scraper )
    {
        return;
    }

    ui->searchStringId->clear();
    if( ( scraper->identifier() == "imdb" ) && ( ! m_id.isEmpty() ) )
    {
        ui->searchStringId->setText( m_id );
    }
    else if(    ( scraper->identifier() == "tmdb" )
             && ( ! m_tmdbId.isEmpty() )
             && ( ! m_tmdbId.startsWith( "tt" ) ) )
    {
        ui->searchStringId->setText( "id" + m_tmdbId );
    }
    else if( ( scraper->identifier() == "tmdb" ) && ( ! m_id.isEmpty() ) )
    {
        ui->searchStringId->setText( m_id );
    }

    bool flag = this->ui->searchStringId->text().isEmpty();
    this->ui->rBtnSearchId->setChecked( ! flag );
    this->ui->rBtnSearchTitle->setChecked( flag );
    this->searchById( ! flag );

    if( scraper->identifier() == "tmdb" )
    {
        this->ui->lblOverview->show();
        this->ui->teOverview->show();
    }
    else
    {
        this->ui->lblOverview->hide();
        this->ui->teOverview->hide();
    }
}


void MovieSearchWidget::onSearchIdentifierSwitched()
{
    qDebug() << Q_FUNC_INFO;

    this->searchById( this->ui->rBtnSearchId->isChecked() );
}


void MovieSearchWidget::searchById( bool flag )
{
    qDebug() << Q_FUNC_INFO << ": flag: " << flag;

    this->ui->searchStringId->setEnabled( flag );
    this->ui->searchStringTitle->setEnabled( ! flag );

    if( flag )
    {
        this->ui->searchStringId->setFocus();
    }
    else
    {
        this->ui->searchStringTitle->setFocus();
    }
}


ScraperInterface* MovieSearchWidget::getCurrentScraper()
{
    qDebug() << Q_FUNC_INFO;

    int index = this->ui->comboScraper->currentIndex();
    m_scraperId = this->ui->comboScraper->itemData( index, UserRoleId ).toString();
    return Manager::instance()->scraper( m_scraperId );
}
