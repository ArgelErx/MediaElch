#include "MakeMkvDialog.h"
#include "ui_MakeMkvDialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include "globals/Helper.h"
#include "globals/Manager.h"
#include "notifications/Notificator.h"
#include "renamer/Renamer.h"

MakeMkvDialog::MakeMkvDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MakeMkvDialog)
{
    ui->setupUi(this);
    ui->stackedWidget->setAnimation(QEasingCurve::Linear);
    ui->stackedWidget->setSpeed(200);

    ui->badgeSuccess->setActive(true);
    ui->badgeSuccess->setBadgeType(Badge::LabelSuccess);
    ui->badgeSuccess->setShowActiveMark(true);

    QMovie *loadingMovie = new QMovie(":/img/spinner.gif");
    loadingMovie->start();
    ui->loading->setMovie(loadingMovie);

    m_makeMkvCon = new MakeMkvCon(this);

    connect(m_makeMkvCon, SIGNAL(sigMessage(QString)), ui->messages, SLOT(appendPlainText(QString)));
    connect(m_makeMkvCon, SIGNAL(sigGotDrives(QMap<int,QString>)), this, SLOT(onGotDrives(QMap<int,QString>)));
    connect(m_makeMkvCon, SIGNAL(sigScannedDrive(QString, QMap<int, MakeMkvCon::Track>)), this, SLOT(onScanFinished(QString, QMap<int, MakeMkvCon::Track>)));
    connect(m_makeMkvCon, SIGNAL(sigDiscBackedUp()), this, SLOT(onDiscBackedUp()));
    connect(m_makeMkvCon, SIGNAL(sigTrackImported(int)), this, SLOT(onTrackImported(int)));
    connect(m_makeMkvCon, SIGNAL(sigProgress(int,int)), this, SLOT(onImportProgress(int,int)));
    connect(ui->btnScanDrive, SIGNAL(clicked()), this, SLOT(onScanDrive()));
    connect(ui->btnClose, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->btnImportTracks, SIGNAL(clicked()), this, SLOT(onImportTracks()));
    connect(ui->btnImportComplete, SIGNAL(clicked()), this, SLOT(onImportComplete()));
    connect(ui->movieSearchWidget, SIGNAL(sigResultClicked()), this, SLOT(onMovieChosen()));
    connect(ui->btnImport, SIGNAL(clicked()), this, SLOT(onImport()));
}

MakeMkvDialog::~MakeMkvDialog()
{
    delete ui;
}

void MakeMkvDialog::reject()
{
    if (!ui->btnClose->isEnabled())
        return;

    if (m_movie) {
        m_movie->controller()->abortDownloads();
        m_movie->deleteLater();
    }

    Settings::instance()->setMakeMkvDialogSize(size());
    Settings::instance()->setMakeMkvDialogPosition(pos());
    Settings::instance()->saveSettings();
    storeDefaults();
    QDialog::reject();
}

void MakeMkvDialog::accept()
{
    if (m_movie) {
        m_movie->controller()->abortDownloads();
        m_movie->deleteLater();
    }

    Settings::instance()->setMakeMkvDialogSize(size());
    Settings::instance()->setMakeMkvDialogPosition(pos());
    Settings::instance()->saveSettings();
    storeDefaults();
    QDialog::accept();
}

int MakeMkvDialog::exec()
{
    if (Settings::instance()->makeMkvDialogSize().isValid() && !Settings::instance()->makeMkvDialogPosition().isNull()) {
        move(Settings::instance()->makeMkvDialogPosition());
        resize(Settings::instance()->makeMkvDialogSize());
    }

    ui->comboImportDir->clear();
    foreach (const SettingsDir &dir, Settings::instance()->movieDirectories())
        ui->comboImportDir->addItem(dir.path, dir.separateFolders);

    ui->btnImport->setVisible(false);
    ui->stackedWidget->setCurrentIndex(0);
    ui->comboDrives->clear();
    ui->btnScanDrive->setEnabled(false);
    ui->messages->clear();
    ui->tracks->clear();
    ui->tracksWidget->setVisible(false);
    ui->messages->setVisible(true);
    ui->labelLoading->setVisible(true);
    ui->btnClose->setEnabled(false);
    ui->progressBar->setValue(0);
    setDefaults();
    m_makeMkvCon->onGetDrives();
    return QDialog::exec();
}

void MakeMkvDialog::storeDefaults()
{
    QString fileName;
    QString fileNameMulti;
    QString directoryName;
    QString seasonName;
    Settings::instance()->renamePatterns(Renamer::TypeMovies, fileName, fileNameMulti, directoryName, seasonName);

    fileName = ui->fileNaming->text();
    fileNameMulti = ui->multiFileNaming->text();
    directoryName = ui->directoryNaming->text();

    Settings::instance()->setRenamePatterns(Renamer::TypeMovies, fileName, fileNameMulti, directoryName, seasonName);
}

void MakeMkvDialog::setDefaults()
{
    QString fileName;
    QString fileNameMulti;
    QString directoryName;
    QString seasonName;
    Settings::instance()->renamePatterns(Renamer::TypeMovies, fileName, fileNameMulti, directoryName, seasonName);
    ui->fileNaming->setText(fileName);
    ui->multiFileNaming->setText(fileNameMulti);
    ui->directoryNaming->setText(directoryName);
}

void MakeMkvDialog::onGotDrives(QMap<int, QString> drives)
{
    QMapIterator<int, QString> it(drives);
    while (it.hasNext()) {
        it.next();
        ui->comboDrives->addItem(it.value(), it.key());
    }
    ui->btnScanDrive->setEnabled(!drives.isEmpty());
    ui->btnClose->setEnabled(true);
    if (!drives.isEmpty())
        onScanDrive();
}

void MakeMkvDialog::onScanDrive()
{
    int index = ui->comboDrives->currentIndex();
    if (index < 0 || index >= ui->comboDrives->count())
        return;

    int id = ui->comboDrives->itemData(index, Qt::UserRole).toInt();
    ui->btnClose->setEnabled(false);
    ui->btnScanDrive->setEnabled(false);
    ui->messages->clear();
    ui->tracksWidget->setVisible(false);
    ui->messages->setVisible(true);
    m_makeMkvCon->onScanDrive(id);
}

void MakeMkvDialog::onScanFinished(QString title, QMap<int, MakeMkvCon::Track> tracks)
{
    QMapIterator<int, MakeMkvCon::Track> it(tracks);
    while (it.hasNext()) {
        it.next();
        QListWidgetItem *item = new QListWidgetItem(QString("%1 (%3, %2)").arg(it.value().name).arg(it.value().duration).arg(Helper::formatFileSize(it.value().size)));
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        item->setData(Qt::UserRole, it.key());
        item->setData(Qt::UserRole+1, it.value().fileName);
        ui->tracks->addItem(item);
    }
    ui->tracksWidget->setVisible(true);
    ui->messages->setVisible(false);
    ui->btnScanDrive->setEnabled(true);
    ui->btnClose->setEnabled(true);
    m_title = title;
}

void MakeMkvDialog::onImportTracks()
{
    m_importComplete = false;
    m_tracks.clear();
    for (int i=0, n=ui->tracks->count() ; i<n ; ++i) {
        if (ui->tracks->item(i)->checkState() == Qt::Checked)
            m_tracks.insert(ui->tracks->item(i)->data(Qt::UserRole).toInt(), ui->tracks->item(i)->data(Qt::UserRole+1).toString());
    }

    if (m_tracks.isEmpty()) {
        QMessageBox::warning(this, tr("No tracks selected"),
                             tr("Please select at least one track you want to import."),
                             QMessageBox::Ok);
        return;
    }

    ui->movieSearchWidget->search(m_title, "", "");
    ui->stackedWidget->slideInIdx(1);
}

void MakeMkvDialog::onImportComplete()
{
    m_importComplete = true;
    m_tracks.clear();
    ui->movieSearchWidget->search(m_title, "", "");
    ui->stackedWidget->slideInIdx(1);
}

void MakeMkvDialog::onMovieChosen()
{
    QMap<ScraperInterface*, QString> ids;
    QList<int> infosToLoad;
    if (ui->movieSearchWidget->scraperId() == "custom-movie") {
        ids = ui->movieSearchWidget->customScraperIds();
        infosToLoad = Settings::instance()->scraperInfos(WidgetMovies, "custom-movie");
    } else {
        ids.insert(0, ui->movieSearchWidget->scraperMovieId());
        infosToLoad = ui->movieSearchWidget->infosToLoad();
    }

    if (m_movie)
        m_movie->deleteLater();

    bool multiFile = (m_tracks.count() > 1);

    ui->loading->setVisible(true);
    ui->labelLoading->setText(tr("Loading movie information..."));
    ui->badgeSuccess->setVisible(false);
    ui->formLayout->setEnabled(false);
    ui->descPart->setVisible(multiFile);
    ui->labelPart->setVisible(multiFile);
    ui->labelMultiFileNaming->setVisible(multiFile);
    ui->multiFileNaming->setVisible(multiFile);
    ui->fileNaming->setVisible(!multiFile);
    ui->labelFileNaming->setVisible(!multiFile);
    ui->fileNaming->setEnabled(!m_importComplete);
    ui->stackedWidget->slideInIdx(2);
    ui->btnImport->setVisible(true);
    ui->btnImport->setEnabled(false);

    m_movie = new Movie(QStringList());
    m_movie->controller()->loadData(ids, Manager::instance()->scraper(ui->movieSearchWidget->scraperId()), infosToLoad);
    connect(m_movie->controller(), SIGNAL(sigLoadDone(Movie*)), this, SLOT(onLoadDone(Movie*)), Qt::UniqueConnection);
}

void MakeMkvDialog::onLoadDone(Movie *movie)
{
    if (movie != m_movie)
        return;

    ui->loading->setVisible(false);
    ui->badgeSuccess->setText(tr("Movie information was loaded"));
    ui->labelLoading->setVisible(false);
    ui->badgeSuccess->setVisible(true);
    ui->btnImport->setEnabled(true);
    ui->formLayout->setEnabled(true);
}

void MakeMkvDialog::onImport()
{
    if (ui->comboImportDir->count() == 0)
        return;

    m_importDir = ui->comboImportDir->currentText();
    QDir dir(ui->comboImportDir->currentText());
    if (ui->comboImportDir->itemData(ui->comboImportDir->currentIndex()).toBool()) {
        QString newFolderName = ui->directoryNaming->text();
        newFolderName.replace("<title>", m_movie->name());
        newFolderName.replace("<originalTitle>", m_movie->originalName());
        newFolderName.replace("<year>", m_movie->released().toString("yyyy"));
        Helper::sanitizeFileName(newFolderName);
        if (!dir.mkdir(newFolderName)) {
            QMessageBox::warning(this, tr("Creating destination directory failed"),
                                 tr("The destination directory %1 could not be created").arg(dir.absolutePath() + QDir::separator() + newFolderName));
            return;
        }
        dir.cd(newFolderName);
        m_importDir = dir.absolutePath();
    }

    int driveId = ui->comboDrives->itemData(ui->comboDrives->currentIndex()).toInt();
    QMapIterator<int, QString> it(m_tracks);
    while (it.hasNext()) {
        it.next();
        m_makeMkvCon->onImportTrack(it.key(), driveId, m_importDir);
        break;
    }
    ui->btnImport->setEnabled(false);
    ui->btnClose->setEnabled(false);
}

void MakeMkvDialog::onDiscBackedUp()
{
    QStringList files;
    if (QFileInfo(m_importDir + "/BDMV/index.bdmv").exists()) {
        files << m_importDir + "/BDMV/index.bdmv";
        m_movie->setDiscType(DiscBluRay);
    } else if (QFileInfo(m_importDir + "/VIDEO_TS/VIDEO_TS.IFO").exists()) {
        files << m_importDir + "/VIDEO_TS/VIDEO_TS.IFO";
        m_movie->setDiscType(DiscDvd);
    }
    m_movie->setFiles(files);
    importFinished();
}

void MakeMkvDialog::onTrackImported(int trackId)
{
    QStringList files = m_movie->files();
    files << m_importDir + "/" + m_tracks[trackId];
    m_movie->setFiles(files);
    m_tracks.remove(trackId);

    int driveId = ui->comboDrives->itemData(ui->comboDrives->currentIndex()).toInt();
    QMapIterator<int, QString> it(m_tracks);
    while (it.hasNext()) {
        it.next();
        m_makeMkvCon->onImportTrack(it.key(), driveId, m_importDir);
        return;
    }

    importFinished();
}

void MakeMkvDialog::importFinished()
{
    if (m_movie->discType() != DiscBluRay && m_movie->discType() != DiscDvd) {
        QStringList files;
        int partNo = 0;
        foreach (QString file, m_movie->files()) {
            QFileInfo fi(file);
            QString newFileName = ui->fileNaming->text();
            if (m_movie->files().count() > 1)
                newFileName = ui->multiFileNaming->text();
            newFileName.replace("<title>", m_movie->name());
            newFileName.replace("<originalTitle>", m_movie->originalName());
            newFileName.replace("<year>", m_movie->released().toString("yyyy"));
            newFileName.replace("<extension>", fi.suffix());
            newFileName.replace("<partNo>", QString::number(++partNo));
            Helper::sanitizeFileName(newFileName);
            QFile f(file);
            f.rename(m_importDir + "/" + newFileName);
            files << m_importDir + "/" + newFileName;
        }
        m_movie->setFiles(files);
    }

    m_movie->setInSeparateFolder(ui->comboImportDir->itemData(ui->comboImportDir->currentIndex()).toBool());
    if (!m_movie->files().isEmpty())
        m_movie->setFileLastModified(QFileInfo(m_movie->files().first()).lastModified());
    m_movie->controller()->loadStreamDetailsFromFile();
    m_movie->controller()->saveData(Manager::instance()->mediaCenterInterface());
    m_movie->controller()->loadData(Manager::instance()->mediaCenterInterface());
    Manager::instance()->database()->add(m_movie, ui->comboImportDir->currentText());
    Manager::instance()->database()->commit();
    Manager::instance()->movieModel()->addMovie(m_movie);
    m_movie = 0;

    Notificator::instance()->notify(Notificator::Information, tr("MakeMKV import finished"), tr("Import with MakeMKV has finished"));

    ui->loading->setVisible(false);
    ui->badgeSuccess->setText(tr("Import has finished"));
    ui->progressBar->setValue(ui->progressBar->maximum());
    ui->btnClose->setEnabled(true);
}

void MakeMkvDialog::onImportProgress(int value, int max)
{
    ui->progressBar->setValue(value);
    ui->progressBar->setMaximum(max);
}
