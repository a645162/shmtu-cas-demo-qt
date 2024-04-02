#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <iostream>
#include <filesystem>

#include "QMessageBox"
#include "QFileDialog"

#include "cross_platform_macro.h"

#include "QtUIHelper.h"

#include "CAS_OCR_QT.h"
#include "CasNetwork.h"

#include "STD_CPP_Utils.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setWindowIcon(QIcon(":/img/logopng.fw.ico"));

    // 禁止窗口最大化
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
    // 禁止调整窗口大小
    this->setFixedSize(this->size());

    setQPushButtonFixedSize(ui->pushButton_DownloadUrl);
    setQPushButtonFixedSize(ui->pushButton_ocr);
    setQPushButtonFixedSize(ui->pushButton_OpenLocal);
    setQPushButtonFixedSize(ui->pushButton_ReleaseModel);
}

MainWindow::~MainWindow() {
    delete ui;

    // Must release model before exit
    CAS_OCR::release_model();
}

void MainWindow::on_pushButton_DownloadUrl_clicked() {
    auto pixmap =
            DownloadQPixmapFromUrl("https://cas.shmtu.edu.cn/cas/captcha");
    SetPixelMapToLabel(ui->label_ImageView, pixmap);
}

void MainWindow::on_pushButton_ocr_clicked() {
    if (_innerBitmap.isNull()) {
        ui->statusbar->showMessage("错误：请先下载或打开图片!");
        QMessageBox msgBox;
        msgBox.setText("请先下载或打开图片!");
        msgBox.setWindowTitle("Result");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);

        msgBox.exec();
        return;
    }

    if (!CAS_OCR::is_model_init()) {
        // 模型加载后就没必要再设置工作设备了，毕竟模型已经加载了移动不了了！
        const auto use_gpu = ui->checkBox_UseGpu->isChecked();
        if (use_gpu) {
            CAS_OCR::get_gpu_info().print_info();
        }
        CAS_OCR::set_model_gpu_support(use_gpu);
    }

    auto checkpoint_path =
            ui->lineEdit_CheckpointPath->text().toStdString();
    std_extend::string_trim(checkpoint_path);

    if (checkpoint_path.empty()) {
#if defined(__APPLE__) && defined(NCNN_APPLE_CHECKPOINT)
        const auto applicationDirPath =
                QCoreApplication::applicationDirPath();
        const auto applicationDirPathStr =
                applicationDirPath.toStdString();
        checkpoint_path = applicationDirPathStr + "/" + NCNN_APPLE_CHECKPOINT;
#endif
    }

    if (checkpoint_path.empty()) {
#ifdef _DEBUG
        // Debug Mode

#if defined(_WINDOWS)
        checkpoint_path =
                R"(H:\SHMTU\shmtu-cas\Deploy\SHMTU_CAS_OCR_Demo_Windows\checkpoint)";
#elif defined(__APPLE__)
        checkpoint_path =
                "/Users/konghaomin/SHMTU/shmtu-cas/Deploy/SHMTU_CAS_OCR_Demo_Android/shmtu_ocr/src/main/assets";
#if defined(TARGET_IPHONE_SIMULATOR)
        // iOS Simulator
#elif defined(TARGET_OS_IPHONE)
        // iOS device
#elif defined(TARGET_OS_MAC)
        // Other kinds of Mac OS
#endif

#elif defined(__ANDROID__)
        // android
#elif defined(__linux__)
        // linux
        checkpoint_path =
                "/media/konghaomin/PM963/SHMTU/shmtu-cas/Deploy/SHMTU_CAS_OCR_Demo_Android/shmtu_ocr/src/main/assets";
#endif

#else
        // Release Mode

#endif
    }

    ui->statusbar->showMessage("正在加载模型...");
    const auto init_result = CAS_OCR::init_model(
        checkpoint_path,
        "fp16"
    );

    if (!init_result) {
        ui->statusbar->showMessage("模型失败!!!");

        QMessageBox msgBox;
        msgBox.setText("模型加载失败!\n请确认路径是否正确！");
        msgBox.setWindowTitle("模型加载失败");
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStandardButtons(QMessageBox::Ok);

        msgBox.exec();
        return;
    }

    ui->statusbar->showMessage("模型加载完毕!正在识别...");

    const auto result =
            CAS_OCR::predict_validate_code(_innerBitmap);

    const auto expr = std::get<1>(result);

    ui->label_Result->setText(QString::fromStdString(expr));
    ui->statusbar->showMessage("OCR识别完毕!");

    QMessageBox msgBox;
    msgBox.setText(QString::fromStdString(expr));
    msgBox.setWindowTitle("Result");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();

    //    CAS_OCR::release_model();
}

void MainWindow::on_pushButton_OpenLocal_clicked() {
    QFileDialog dialog;

    // 设置对话框标题
    dialog.setWindowTitle("Choose Image File");

    // 设置对话框的显示模式为只选择单个文件
    dialog.setFileMode(QFileDialog::ExistingFile);

    // 设置文件过滤器，只允许选择 .png、.jpg 和 .bmp 格式的文件
    QStringList filters;
    filters << "Image files (*.png *.jpg *jpeg *.bmp)";
    dialog.setNameFilters(filters);

    // 显示文件选择对话框，并等待用户选择文件
    if (dialog.exec()) {
        // 获取用户选择的文件路径
        QString selectedFile = dialog.selectedFiles().value(0);

        // 处理选择的文件，这里可以进行后续操作，比如读取文件内容等
        qDebug() << "Selected file:" << selectedFile;

        // 加载图片
        const QPixmap pixmap(selectedFile);

        if (pixmap.isNull()) {
            std::cerr << "Pixmap is null!" << std::endl;
            qDebug() << "Pixmap is empty!";

            ui->statusbar->showMessage("错误：打开图片失败!");
            QMessageBox msgBox;
            msgBox.setText("错误：打开图片失败!");
            msgBox.setWindowTitle("Result");
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setStandardButtons(QMessageBox::Ok);

            msgBox.exec();
            return;
        }

        SetPixelMapToLabel(ui->label_ImageView, pixmap);
    }
}

void MainWindow::on_pushButton_ReleaseModel_clicked() {
    void ReleaseModel();
    ReleaseModel();
}

void MainWindow::SetPixelMapToLabel(QLabel *label, const QPixmap &pixmap) {
    if (pixmap.isNull()) {
        std::cerr << "Pixmap is null!" << std::endl;
        // qDebug() << "Pixmap is empty!";
    } else {
#ifdef _DEBUG
        std::cout << "Pixmap is correct!" << std::endl;
#endif
        // qDebug() << "Pixmap is correct!";
    }

    _innerBitmap = pixmap.copy();

    // 将图片设置到 QLabel 中
    label->setPixmap(pixmap);

    // 设置图片随着 QLabel 大小自动缩放
    label->setScaledContents(true);
}

void MainWindow::on_actionReleaseModel_triggered() {
    void ReleaseModel();
    ReleaseModel();
}

void MainWindow::on_actionExit_triggered() {
    close();
}

void MainWindow::on_actionDownload_From_URL_triggered() {
    on_pushButton_DownloadUrl_clicked();
}


void MainWindow::on_actionOpenLocal_triggered() {
    on_pushButton_OpenLocal_clicked();
}

void MainWindow::on_actionDoOCR_triggered() {
    on_pushButton_ocr_clicked();
}

void MainWindow::on_actionAbout_triggered() {
    const auto qtVersion =
            QString::fromStdString(qVersion());
    const auto applicationDirPath =
            QCoreApplication::applicationDirPath();

    const auto currentDir =
            std::filesystem::current_path();
    const auto currentDirStr =
            currentDir.string();
    const auto currentDirQStr =
            QString::fromStdString(currentDirStr);

    QMessageBox msgBox;
    msgBox.setText(
        "上海海事大学统一认证平台验证码OCR识别Demo\n"
        "Author:Haomin Kong\n"
        "这是孔昊旻同学的一个课程设计。\n"
        "本程序支持Windows/macOS/Linux\n"
        "请勿用于非法用途，以及商业用途！！！\n"
        "Qt version:" + qtVersion + "\n"
        "Application Dir Path:" + applicationDirPath + "\n"
        "Current Path:" + currentDirQStr + "\n"
    );
    msgBox.setWindowTitle("About");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();
}

void ReleaseModel() {
    CAS_OCR::release_model();

    QMessageBox msgBox;
    msgBox.setText(
        "模型已释放！"
    );
    msgBox.setWindowTitle("Info");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);

    msgBox.exec();
}
