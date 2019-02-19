//------------------------------------------------------------------------
//  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------

#include <config.h>
#include <QApplication>
#include <QtGlobal>
#include <QWidget>
#include <QLayout>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QTextEdit>
#include <QFileDialog>
#include <QImage>
#include <zbar/QZBar.h>
#include <zbar.h>

#define TEST_IMAGE_FORMATS \
    "Image Files (*.png *.jpg *.jpeg *.bmp *.gif *.ppm *.pgm *.pbm *.tiff *.xpm *.xbm)"

#define SYM_GROUP "Symbology"
#define CAM_GROUP "Camera"
#define DBUS_NAME "D-Bus"
#define OPTION_BAR "option_bar.enable"
#define CONTROL_BAR "control_bar.enable"

extern "C" {
int scan_video(void *add_device,
               void *userdata,
               const char *default_device);
}

struct configs_s {
    QString name;
    zbar::zbar_symbol_type_t sym;
};

static const struct configs_s configs[] = {
    { "Composite codes", zbar::ZBAR_COMPOSITE },
    { "Image Scanner", zbar::ZBAR_PARTIAL },
#if ENABLE_CODABAR == 1
    { "Codabar", zbar::ZBAR_CODABAR },
#endif
#if ENABLE_CODE128 == 1
    { "Code-128", zbar::ZBAR_CODE128 },
#endif
#if ENABLE_I25 == 1
    { "Code 2 of 5 interlaced", zbar::ZBAR_I25 },
#endif
#if ENABLE_CODE39 == 1
    { "Code-39", zbar::ZBAR_CODE39 },
#endif
#if ENABLE_CODE93 == 1
    { "Code-93", zbar::ZBAR_CODE93 },
#endif
#if ENABLE_DATABAR == 1
    { "DataBar", zbar::ZBAR_DATABAR },
    { "DataBar expanded", zbar::ZBAR_DATABAR_EXP },
#endif
#if ENABLE_EAN == 1
    { "EAN-2", zbar::ZBAR_EAN2 },
    { "EAN-5", zbar::ZBAR_EAN5 },
    { "EAN-8", zbar::ZBAR_EAN8 },
    { "EAN-13", zbar::ZBAR_EAN13 },
    { "ISBN-10", zbar::ZBAR_ISBN10 },
    { "ISBN-13", zbar::ZBAR_ISBN13 },
    { "UPC-A", zbar::ZBAR_UPCA },
    { "UPC-E", zbar::ZBAR_UPCE },
#endif
#if ENABLE_PDF417 == 1
    { "PDF417", zbar::ZBAR_PDF417 },
#endif
#if ENABLE_QRCODE == 1
    { "QR code", zbar::ZBAR_QRCODE },
#endif
#if ENABLE_SQCODE == 1
    { "SQ code", zbar::ZBAR_SQCODE },
#endif
};

#define CONFIGS_SIZE (sizeof(configs)/sizeof(*configs))

struct settings_s {
    QString name;
    zbar::zbar_config_t ctrl;
    bool is_bool;
};

static const struct settings_s settings[] = {
    { "x-density",     zbar::ZBAR_CFG_Y_DENSITY,     false },
    { "y-density",     zbar::ZBAR_CFG_Y_DENSITY,     false },
    { "min-length",    zbar::ZBAR_CFG_MIN_LEN,       false },
    { "max-length",    zbar::ZBAR_CFG_MAX_LEN,       false },
    { "uncertainty",   zbar::ZBAR_CFG_UNCERTAINTY,   false },
    { "ascii",         zbar::ZBAR_CFG_ASCII,         true },
    { "add-check",     zbar::ZBAR_CFG_ADD_CHECK,     true },
    { "emit-check",    zbar::ZBAR_CFG_EMIT_CHECK,    true },
    { "position",      zbar::ZBAR_CFG_POSITION,      true },
    { "test-inverted", zbar::ZBAR_CFG_TEST_INVERTED, true },
};
#define SETTINGS_SIZE (sizeof(settings)/sizeof(*settings))

// Represents an integer control

class IntegerControl : public QSpinBox
{
    Q_OBJECT

private:
    char *name;
    zbar::QZBar *zbar;

private slots:
    void updateControl(int value);

public:

    IntegerControl(QGroupBox *parent, zbar::QZBar *_zbar, char *_name,
                   int min, int max, int def, int step)
        : QSpinBox(parent)
    {
        int val;

        zbar = _zbar;
        name = _name;

        setRange(min, max);
        setSingleStep(step);
        if (!zbar->get_control(name, &val))
            setValue(val);
        else
            setValue(def);

        connect(this, SIGNAL(valueChanged(int)),
                this, SLOT(updateControl(int)));
    }
};

void IntegerControl::updateControl(int value)
{
        zbar->set_control(name, value);
}

// Represents a menu control
class MenuControl : public QComboBox
{
    Q_OBJECT

private:
    char *name;
    zbar::QZBar *zbar;
    QVector< QPair< int , QString > > vector;

private slots:
    void updateControl(int value);

public:

    MenuControl(QGroupBox *parent, zbar::QZBar *_zbar, char *_name,
                QVector< QPair< int , QString > > _vector)
        : QComboBox(parent)
    {
        int val;

        zbar = _zbar;
        name = _name;
        vector = _vector;

        if (zbar->get_control(name, &val))
            val = 0;
        for (int i = 0; i < vector.size(); ++i) {
            QPair < int , QString > pair = vector.at(i);
            addItem(pair.second, pair.first);

            if (val == pair.first)
                setCurrentIndex(i);
        }
        connect(this, SIGNAL(currentIndexChanged(int)),
                this, SLOT(updateControl(int)));
    }
};

void MenuControl::updateControl(int index)
{

    zbar->set_control(name, vector.at(index).first);
}

class IntegerSetting : public QSpinBox
{
    Q_OBJECT

public:
    QString name;

    IntegerSetting(QString _name, int val = 0) : name(_name) {
        setValue(val);
    }
};

class SettingsDialog : public QDialog
{
    Q_OBJECT

private:
    QVector<int> val;
    zbar::QZBar *zbar;
    zbar::zbar_symbol_type_t sym;

private slots:

    void accept() {
        for (unsigned i = 0; i < SETTINGS_SIZE; i++)
            zbar->set_config(sym, settings[i].ctrl, val[i]);
        QDialog::accept();
    };
    void reject() {
        QDialog::reject();
    };
    void clicked() {
        QCheckBox *button = qobject_cast<QCheckBox*>(sender());
        if (!button)
            return;

        QString name = button->text();

        for (unsigned i = 0; i < SETTINGS_SIZE; i++) {
            if (settings[i].name == name) {
                val[i] = button->isChecked();
                return;
            }
        }
        // ERROR!
    };
    void update(int value) {
        IntegerSetting *setting = qobject_cast<IntegerSetting*>(sender());
        if (!setting)
            return;

        for (unsigned i = 0; i < SETTINGS_SIZE; i++) {
            if (settings[i].name == setting->name) {
                val[i] = value;
                return;

            }
        }
        // ERROR!
    };

public:
    SettingsDialog(zbar::QZBar *_zbar, QString &name, zbar::zbar_symbol_type_t _sym)
                  : zbar(_zbar), sym(_sym) {
        val = QVector<int>(SETTINGS_SIZE);

        QGridLayout *layout = new QGridLayout(this);

        this->setWindowTitle(name);

        for (unsigned i = 0; i < SETTINGS_SIZE; i++) {
            int value = 0;

            if (zbar->get_config(sym, settings[i].ctrl, value))
                continue;
            val[i] = value;

            if (settings[i].is_bool) {
                QCheckBox *button = new QCheckBox(settings[i].name, this);

                button->setChecked(value);

                layout->addWidget(button, i, 0, 1, 2,
                                  Qt::AlignTop | Qt::AlignLeft);
                connect(button, SIGNAL(clicked()),
                        this, SLOT(clicked()));
            } else {
                QLabel *label = new QLabel(settings[i].name);

                layout->addWidget(label, i, 0, 1, 1,
                                  Qt::AlignTop | Qt::AlignLeft);
                IntegerSetting *spin = new IntegerSetting(settings[i].name, value);
                layout->addWidget(spin, i, 1, 1, 1,
                                  Qt::AlignTop | Qt::AlignLeft);
                connect(spin, SIGNAL(valueChanged(int)),
                        this, SLOT(update(int)));
            }

        }
        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
        layout->addWidget(buttonBox);
    }
};

class SettingsButton : public QPushButton
{
    Q_OBJECT

private:
    QString name;
    zbar::QZBar *zbar;
    zbar::zbar_symbol_type_t sym;

public:
    SettingsButton(zbar::QZBar *_zbar, const QIcon &_icon, QString _name,
                   zbar::zbar_symbol_type_t _sym)
                  : QPushButton(_icon, ""),
                    name(_name), zbar(_zbar), sym(_sym)
    {
        int size = font().pointSize();

        if (size < 0)
            size = font().pixelSize();

        if (size > 0)
            setIconSize(QSize(size, size));
    };

public Q_SLOTS:
    void button_clicked()
    {

        SettingsButton *button = qobject_cast<SettingsButton*>(sender());
        if (!button)
            return;

        QString name = button->name;

        SettingsDialog *dialog = new SettingsDialog(zbar, name, sym);
        dialog->setModal(true);
        dialog->show();
    }
};

struct CamRes {
    unsigned width;
    unsigned height;
    float max_fps;
};

class ZbarcamQZBar : public QWidget
{
    Q_OBJECT

protected:
    static void add_device (QComboBox *list,
                            const char *device)
    {
        list->addItem(QString(device));
    }

public Q_SLOTS:
    void turn_show_options()
    {
        QPushButton *button = qobject_cast<QPushButton*>(sender());
        if (!button)
            return;

        show_options = !show_options;
        if (show_options) {
            button->setText("Hide Options");
            optionsGroup->show();
        } else {
            button->setText("Show Options");
            optionsGroup->hide();
        }
    }

    void turn_show_controls()
    {
        QPushButton *button = qobject_cast<QPushButton*>(sender());
        if (!button)
            return;

        show_controls = !show_controls;
        if (show_controls) {
            button->setText("Hide Controls");
            controlGroup->show();
        } else {
            button->setText("Show Controls");
            controlGroup->hide();
        }
    }

public:
    ~ZbarcamQZBar () {
        saveSettings();
    }
    ZbarcamQZBar (const char *default_device, int verbose = 0)
                 : resolutions(NULL)
    {
        // drop-down list of video devices
        QComboBox *videoList = new QComboBox;

        // toggle button to disable/enable video
        statusButton = new QPushButton;

        QStyle *style = QApplication::style();
        QIcon statusIcon = style->standardIcon(QStyle::SP_DialogNoButton);
        QIcon yesIcon = style->standardIcon(QStyle::SP_DialogYesButton);
        statusIcon.addPixmap(yesIcon.pixmap(QSize(128, 128),
                                            QIcon::Normal, QIcon::On),
                             QIcon::Normal, QIcon::On);

        statusButton->setIcon(statusIcon);
        statusButton->setText("&Enable");
        statusButton->setCheckable(true);
        statusButton->setEnabled(false);
        statusButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // command button to open image files for scanning
        QPushButton *openButton = new QPushButton("&Open");
        QIcon openIcon = style->standardIcon(QStyle::SP_DialogOpenButton);
        openButton->setIcon(openIcon);
        openButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        // collect video list and buttons horizontally
        ZBarMenu = new QHBoxLayout;
        ZBarMenu->setAlignment(Qt::AlignLeft);
        ZBarMenu->addWidget(videoList, 5);
        ZBarMenu->addWidget(statusButton, 1);
        ZBarMenu->addWidget(openButton, 1);

        // video barcode scanner
        zbar = new zbar::QZBar(NULL, verbose);
        zbar->setAcceptDrops(true);

        // text box for results
        QTextEdit *results = new QTextEdit;
        results->setReadOnly(true);

        QGridLayout *grid = new QGridLayout;
        grid->addLayout(ZBarMenu, 0, 0, 1, -1);
        grid->addWidget(zbar, 1, 0, 1, 1);
        grid->addWidget(results, 2, 0, 1, 1);

        // Group box where controls will be added
        optionsGroup = new QGroupBox(tr("Options"), this);
        QGridLayout *optionsBoxLayout = new QGridLayout(optionsGroup);
        optionsGroup->setAlignment(Qt::AlignHCenter);
        optionsBoxLayout->setContentsMargins(0, 0, 16, 0);
        grid->addWidget(optionsGroup, 1, 1, -1, 1, Qt::AlignTop);

        controlGroup = new QGroupBox(this);
        controlBoxLayout = new QGridLayout(controlGroup);
        controlBoxLayout->setContentsMargins(0, 0, 0, 0);
        grid->addWidget(controlGroup, 1, 2, -1, 1, Qt::AlignTop);

        loadSettings();
        zbar->request_size(curWidth, curHeight, false);

        int pos = 0;

#ifdef HAVE_DBUS
        QCheckBox *button = new QCheckBox(DBUS_NAME, this);
        button->setChecked(dbus_enabled);
        optionsBoxLayout->addWidget(button, ++pos, 0, 1, 1,
                                    Qt::AlignTop | Qt::AlignLeft);
        connect(button, SIGNAL(clicked()), this, SLOT(code_clicked()));
        zbar->request_dbus(0);
#endif

        for (unsigned i = 0; i < CONFIGS_SIZE; i++) {
            int val = 0;

            if (configs[i].sym == zbar::ZBAR_PARTIAL) {
                QLabel *label = new QLabel(configs[i].name, this);
                optionsBoxLayout->addWidget(label, ++pos, 0, 1, 1,
                                            Qt::AlignTop | Qt::AlignLeft);
            } else {
                QCheckBox *button = new QCheckBox(configs[i].name, this);

                zbar->get_config(configs[i].sym, zbar::ZBAR_CFG_ENABLE, val);

                button->setChecked(val);
                optionsBoxLayout->addWidget(button, ++pos, 0, 1, 1,
                                            Qt::AlignTop | Qt::AlignLeft);
                connect(button, SIGNAL(clicked()), this, SLOT(code_clicked()));
            }

            /* Composite doesn't have configuration */
            if (configs[i].sym == zbar::ZBAR_COMPOSITE)
                continue;

            QIcon icon = QIcon::fromTheme(QLatin1String("configure-toolbars"));
            SettingsButton *settings = new SettingsButton(zbar, icon,
                                                          configs[i].name,
                                                          configs[i].sym);
            optionsBoxLayout->addWidget(settings, pos, 1, 1, 1,
                                        Qt::AlignTop | Qt::AlignLeft);

            connect(settings, &SettingsButton::clicked,
                    settings, &SettingsButton::button_clicked);
        }

        // Allow showing/hiding options/controls menus
        QPushButton *showOptionsButton, *showControlsButton;

        if (show_options) {
            showOptionsButton = new QPushButton("Hide Options");
            optionsGroup->show();
        } else {
            showOptionsButton = new QPushButton("Show Options");
            optionsGroup->hide();
        }
        showOptionsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ZBarMenu->addWidget(showOptionsButton);
        connect(showOptionsButton, SIGNAL(clicked()), this, SLOT(turn_show_options()));

        if (show_controls) {
            showControlsButton = new QPushButton("Hide Controls");
            controlGroup->show();
        } else {
            showControlsButton = new QPushButton("Show Controls");
            controlGroup->hide();
        }
        showControlsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        ZBarMenu->addWidget(showControlsButton);
        connect(showControlsButton, SIGNAL(clicked()), this, SLOT(turn_show_controls()));

        if (!geometry.isEmpty())
            restoreGeometry(geometry);

        setLayout(grid);

        videoList->addItem("");
        int active = scan_video((void*)add_device, videoList, default_device);

        // directly connect combo box change signal to scanner video open
        connect(videoList, SIGNAL(currentIndexChanged(const QString&)),
                zbar, SLOT(setVideoDevice(const QString&)));

        // directly connect status button state to video enabled state
        connect(statusButton, SIGNAL(toggled(bool)),
                zbar, SLOT(setVideoEnabled(bool)));

        // also update status button state when video is opened/closed
        connect(zbar, SIGNAL(videoOpened(bool)),
                this, SLOT(setEnabled(bool)));

        // prompt for image file to scan when openButton is clicked
        connect(openButton, SIGNAL(clicked()), SLOT(openImage()));

        // directly connect video scanner decode result to display in text box
        connect(zbar, SIGNAL(decodedText(const QString&)),
                results, SLOT(append(const QString&)));

        if(active >= 0)
            videoList->setCurrentIndex(active);
    }

public Q_SLOTS:
    void openImage ()
    {
        file = QFileDialog::getOpenFileName(this, "Open Image", file,
                                            TEST_IMAGE_FORMATS);
        if(!file.isEmpty())
            zbar->scanImage(QImage(file));
    }

    void control_clicked()
    {
        QCheckBox *button = qobject_cast<QCheckBox*>(sender());
        if (!button)
            return;

        QString name = button->text();
        bool val = button->isChecked();

        zbar->set_control(name.toUtf8().data(), val);
    }

    void code_clicked()
    {
        QCheckBox *button = qobject_cast<QCheckBox*>(sender());
        if (!button)
            return;

        QString name = button->text();
        bool val = button->isChecked();

        if (name == DBUS_NAME) {
            zbar->request_dbus(val);
            dbus_enabled = val;
            return;
        }

        for (unsigned i = 0; i < CONFIGS_SIZE; i++) {
            if (configs[i].name == name) {
               zbar->set_config(configs[i].sym, zbar::ZBAR_CFG_ENABLE,
                                val);
               return;

            }
        }
    }

    void clearLayout(QLayout *layout)
    {
        QLayoutItem *item;
        while((item = layout->takeAt(0))) {
            if (item->layout()) {
                clearLayout(item->layout());
                delete item->layout();
            }
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
    }

    void setVideoResolution(int index)
    {
        struct CamRes *cur_res;

        if (index < 0 || res.isEmpty())
            return;

        cur_res = &res[index];

        unsigned width = zbar->videoWidth();
        unsigned height = zbar->videoHeight();

        if (width == cur_res->width && height == cur_res->height)
            return;

        zbar->request_size(cur_res->width, cur_res->height);

        curWidth = cur_res->width;
        curHeight = cur_res->height;
    }

    void setEnabled(bool videoEnabled)
    {
        zbar->setVideoEnabled(videoEnabled);

        // Update the status button
        statusButton->setEnabled(videoEnabled);
        statusButton->setChecked(videoEnabled);

        // Delete items before creating a new set of controls
        clearLayout(controlBoxLayout);

        if (!videoEnabled)
            return;

        // get_controls
        loadSettings(false);

        // FIXME: clear a previous resolutions box

        bool isNewResolutions = false;

        if (!resolutions) {
            resolutions = new QComboBox;
            resolutions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            isNewResolutions = true;
        }

        resolutions->blockSignals(true);
        res.clear();
        resolutions->clear();

        for (int i = 0;; i++) {
            QString new_res;
            struct CamRes cur_res;

            if (!zbar->get_resolution(i, cur_res.width, cur_res.height, cur_res.max_fps))
                break;

            new_res.sprintf("%dx%d - %.2f fps (max)",
                            cur_res.width, cur_res.height,
                            cur_res.max_fps);

            resolutions->addItem(new_res);
            res.append(cur_res);

            if (curWidth == cur_res.width && curHeight == cur_res.height)
                resolutions->setCurrentIndex(i);
        }

        if (isNewResolutions) {
            ZBarMenu->addWidget(resolutions);
            connect(resolutions, SIGNAL(currentIndexChanged(int)),
                    this, SLOT(setVideoResolution(int)));
        }
        resolutions->blockSignals(false);

        // Restore saved resolution
        unsigned width = zbar->videoWidth();
        unsigned height = zbar->videoHeight();

        if (width != curWidth || height != curHeight)
        {
            for (int i = 0; i < res.size(); i++) {
                if (res[i].width == curWidth && res[i].height == curHeight) {
                    resolutions->setCurrentIndex(i);
                    break;
                }
            }
        }

        int pos = 0;
        QString oldGroup = "";
        for (int i = 0;; i++) {
            char *name, *group;
            enum zbar::QZBar::ControlType type;
            int min, max, def, step;

            int ret = zbar->get_controls(i, &name, &group, &type,
                                         &min, &max, &def, &step);
            if (!ret)
                break;

            QString newGroup = "<strong>" + QString::fromUtf8(group) +
                               " Controls</strong>";

            if (newGroup != oldGroup) {
                if (oldGroup != "")
                    controlBoxLayout->addItem(new QSpacerItem(0, 12),
                                              pos++, 2, 1, 2,
                                              Qt::AlignLeft);
                QLabel *label = new QLabel(newGroup);
                controlBoxLayout->addWidget(label, pos++, 0, 1, 2,
                                            Qt::AlignTop |
                                            Qt::AlignHCenter);
                pos++;
                oldGroup = newGroup;
            }

            switch (type) {
                case zbar::QZBar::Button:
                case zbar::QZBar::Boolean: {
                    bool val;

                    QCheckBox *button = new QCheckBox(name, controlGroup);
                    controlBoxLayout->addWidget(button, pos++, 0, 1, 2,
                                                Qt::AlignLeft);

                    if (!zbar->get_control(name, &val))
                        button->setChecked(val);
                    else
                        button->setChecked(def);
                    connect(button, SIGNAL(clicked()),
                            this, SLOT(control_clicked()));
                    break;
                }
                case zbar::QZBar::Integer: {
                    IntegerControl *ctrl;

                    QLabel *label = new QLabel(QString::fromUtf8(name));
                    ctrl= new IntegerControl(controlGroup, zbar, name,
                                             min, max, def, step);

                    controlBoxLayout->addWidget(label, pos, 0, Qt::AlignLeft);
                    controlBoxLayout->addWidget(ctrl, pos++, 1, Qt::AlignLeft);
                    break;
                }
                case zbar::QZBar::Menu: {
                    MenuControl *ctrl;

                    QLabel *label = new QLabel(QString::fromUtf8(name));

                    QVector< QPair< int , QString > > vector;
                    vector = zbar->get_menu(i);
                    ctrl= new MenuControl(controlGroup, zbar, name, vector);

                    controlBoxLayout->addWidget(label, pos, 0, Qt::AlignLeft);
                    controlBoxLayout->addWidget(ctrl, pos++, 1, Qt::AlignLeft);
                    break;
                }
                default:
                    // Just ignore other types
                    break;
            }
        }
    }

private:
    QString file;
    zbar::QZBar *zbar;
    QHBoxLayout *ZBarMenu;
    QPushButton *statusButton;
    QGroupBox *controlGroup, *optionsGroup;
    QComboBox *resolutions;
    QGridLayout *controlBoxLayout;
    QSignalMapper *signalMapper;
    bool dbus_enabled, show_options, show_controls;
    QByteArray geometry;
    QVector < struct CamRes > res;
    unsigned curWidth, curHeight;

    void loadSettings(bool getRes = true)
    {
        QSettings qSettings(QCoreApplication::organizationName(),
                            QCoreApplication::applicationName());
        QString key;
        QVariant qVal;

        geometry = qSettings.value("geometry").toByteArray();

        key = OPTION_BAR;
        qVal = qSettings.value(key, true);
        show_options = qVal.toBool();

        key = CONTROL_BAR;
        qVal = qSettings.value(key, true);
        show_controls = qVal.toBool();

        if (getRes) {
            qVal = qSettings.value("width");
            curWidth = qVal.toUInt();
            qVal = qSettings.value("height");
            curHeight = qVal.toUInt();
        }

#ifdef HAVE_DBUS
        key = DBUS_NAME ".enable";
        qVal = qSettings.value(key, false);
        dbus_enabled = qVal.toBool();
#endif

        qSettings.beginGroup(SYM_GROUP);

        for (unsigned i = 0; i < CONFIGS_SIZE; i++) {
            int val = 0;
            if (zbar->get_config(configs[i].sym, zbar::ZBAR_CFG_ENABLE, val))
                continue;
            key = QString(configs[i].name) + QString(".enable");
            key.replace(" ","_");
            qVal = qSettings.value(key, val);
            zbar->set_config(configs[i].sym, zbar::ZBAR_CFG_ENABLE, qVal.toInt());

            if (configs[i].sym == zbar::ZBAR_COMPOSITE)
                continue;

            for (unsigned j = 0; j < SETTINGS_SIZE; j++) {
                int val = 0;

                if (zbar->get_config(configs[i].sym, settings[j].ctrl, val))
                    continue;
                key = QString(configs[i].name) + QString(".") + QString(settings[j].name);
                key.replace(" ","_");

                qVal = qSettings.value(key, val);
                zbar->set_config(configs[i].sym, settings[j].ctrl, qVal.toInt());
            }
        }
        qSettings.endGroup();

        qSettings.beginGroup(CAM_GROUP);
        for (unsigned i = 0;; i++) {
            char *name, *group;
            enum zbar::QZBar::ControlType type;
            int min, max, def, step, val;

            int ret = zbar->get_controls(i, &name, &group, &type,
                                         &min, &max, &def, &step);
            if (!ret)
                break;

            switch (type) {
                case zbar::QZBar::Button:
                case zbar::QZBar::Boolean:
                case zbar::QZBar::Menu:
                case zbar::QZBar::Integer: {
                    key = QString::fromUtf8(name);

                    if (zbar->get_control(name, &val))
                        continue;

                    key.replace(QRegularExpression("[^\\w\\d]+"),"_");
                    key.replace(QRegularExpression("_$"),"");

                    qVal = qSettings.value(key, val);
                    zbar->set_control(name, qVal.toInt());
                    break;
                }
                default:
                    // Just ignore other types
                    break;
            }
        }
        qSettings.endGroup();
    }

    void saveSettings()
    {
        QSettings qSettings(QCoreApplication::organizationName(),
                            QCoreApplication::applicationName());
        QString key;
        unsigned int i;

        qSettings.setValue("geometry", saveGeometry());

        key = OPTION_BAR;
        qSettings.setValue(key, show_options);

        key = CONTROL_BAR;
        qSettings.setValue(key, show_controls);

        curWidth = zbar->videoWidth();
        curHeight = zbar->videoHeight();
        qSettings.setValue("width", curWidth);
        qSettings.setValue("height", curHeight);

#ifdef HAVE_DBUS
        // FIXME: track dbus enable-disable and store last state
        key = DBUS_NAME ".enable";
        qSettings.setValue(key, dbus_enabled);
#endif

        qSettings.beginGroup(SYM_GROUP);
        for (i = 0; i < CONFIGS_SIZE; i++) {
            int val = 0;

            if (zbar->get_config(configs[i].sym, zbar::ZBAR_CFG_ENABLE, val))
                continue;
            key = QString(configs[i].name) + QString(".enable");
            key.replace(" ","_");
            qSettings.setValue(key, val);

            if (configs[i].sym == zbar::ZBAR_COMPOSITE)
                continue;

            for (unsigned j = 0; j < SETTINGS_SIZE; j++) {
                int val = 0;

                if (zbar->get_config(configs[i].sym, settings[j].ctrl, val))
                    continue;
                key = QString(configs[i].name) + QString(".") + QString(settings[j].name);
                key.replace(" ","_");
                qSettings.setValue(key, val);
            }
        }
        qSettings.endGroup();

        for (i = 0;; i++) {
            char *name, *group;
            enum zbar::QZBar::ControlType type;
            int min, max, def, step, val;

            int ret = zbar->get_controls(i, &name, &group, &type,
                                         &min, &max, &def, &step);
            if (!ret)
                break;

            if (i == 0)
                qSettings.beginGroup(CAM_GROUP);

            switch (type) {
                case zbar::QZBar::Button:
                case zbar::QZBar::Boolean:
                case zbar::QZBar::Menu:
                case zbar::QZBar::Integer: {
                    key = QString::fromUtf8(name);

                    if (zbar->get_control(name, &val))
                        continue;

                    key.replace(QRegularExpression("[^\\w\\d]+"),"_");
                    key.replace(QRegularExpression("_$"),"");
                    qSettings.setValue(key, val);
                    break;
                }
                default:
                    // Just ignore other types
                    break;
            }
        }
        if (i > 0)
            qSettings.endGroup();
    }
};

#include "moc_zbarcam_qt.h"

int main (int argc, char *argv[])
{
    int verbose = 0;
    QApplication app(argc, argv);
    app.setApplicationName("zbarcam_qt");
    app.setOrganizationName("LinuxTV");
    app.setOrganizationDomain("linuxtv.org");
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    const char *dev = NULL;

    // FIXME: poor man's argument parser.
    // Should use, instead, QCommandLineParser
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--debug")) {
            verbose = 127;
        } else if (!strcmp(argv[i], "-v")) {
            verbose++;
        } else if (!strcmp(argv[i], "--help")) {
            qWarning() << "Usage:" << argv[0]
                    << "[<--debug>] [<-v>] [<--help>] [<device or file name>]\n";
            return(-1);
        } else {
            dev = argv[i];
        }
    }

    ZbarcamQZBar window(dev, verbose);
    window.show();
    return(app.exec());
}
