#include <QStandardItemModel>
#include <QDebug>
#include <QScrollArea>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollBar>
#include <QLabel>
#include <QTextEdit>
#include <QMessageBox>

#include "dialog.h"
#include "ui_dialog.h"
#include "spinboxdelegate.h"
#include "alg_ahp.h"



Dialog::Dialog(QVector<double> vals, QStringList& list, int index,
               std::vector<std::vector<double>> CR, QWidget* parent):
    QDialog(parent),
    ui(new Ui::Dialog)
{
    qDebug() << "output";
    qDebug() << vals.size();
    qDebug() << list.size();
    ui->setupUi(this);
    this->setMinimumSize(500, 500);
    QVBoxLayout* mainLayout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    mainLayout->setMargin(20);
    mainLayout->addWidget(new QLabel("Ответ"));

    QTextEdit* text = new QTextEdit("\n");
    text->setReadOnly(true);

    text->append("Комбинированные весовые коэффициенты");
    for (int i = 0; i < vals.size(); i++)
    {
        text->append(list[i] + "\t" + QString::number(vals[i]));
    }

    if (index >= 0)
        text->append("Лучший вариант: \"" + list[index] + "\" \t" + QString::number(vals[index]));
    mainLayout->addWidget(text);

    text->append("\n\n\nКоэффициенты согласованности (CR) для каждой матрицы");
    int count = 1;
    for (uint i = 0; i < CR.size(); ++i) {
        for (uint j = 0; j < CR[i].size(); ++j) {
            text->append(QVariant(count++).toString() + ") " + QVariant(CR[i][j]).toString());
        }
    }
}



Dialog::Dialog(const QList<QList<QStringList>>& names, QVector<int> nums, int alternatives, QWidget* parent) :
    QDialog(parent, Qt::Window),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    this->setMinimumSize(1000, 600);
    this->slist = names;
    this->levels = nums.size();
    this->alternatives = alternatives;
    qDebug() << nums;

    QVBoxLayout* mainLayout = new QVBoxLayout(ui->scrollAreaWidgetContents);
    ui->scrollAreaWidgetContents->setLayout(mainLayout);

    mainLayout->setObjectName("mainLayout");

    QVector<QTableView*> vector;


    // Построение TableView для каждого уровня декомпозиции.
    for (int i = 0; i < levels; i++)
    {
        QScrollArea* scroll = new QScrollArea(ui->scrollAreaWidgetContents);
        scroll->setWidgetResizable(true);
        QWidget* widget = new QWidget();
        QHBoxLayout* hlayout = new QHBoxLayout(widget);
        scroll->setWidget(widget);
        mainLayout->addWidget(scroll);

        //DEBUG:
        scroll->setObjectName("scroll_" + QString::number(i));
        widget->setObjectName("widget_" + QString::number(i));
        hlayout->setObjectName("hlayout_" + QString::number(i));


        int tables = (i > 0) ? nums[i - 1] : 1;

        vector.clear();

        for (int j = 0; j < tables; j++)
        {
            QTableView* table = new QTableView();
            QStandardItemModel* model = new QStandardItemModel(table);
            model->setRowCount(nums[i]);
            model->setColumnCount(nums[i]);
            table->setModel(model);
            table->setMinimumWidth(250);
            table->setMinimumHeight(200);
            SpinBoxDelegate* delegate =
                    new SpinBoxDelegate(model->rowCount(), model->columnCount(), this);
            table->setItemDelegate(delegate);

            //TODO индексы неверные
            if (i > 0)
            {
                hlayout->addWidget(new QLabel(this->slist[i - i][0][j]));
                QLabel* lbl = getLabel(":/pics/indicator_gray.svg", {10, 10, 30, 30}, this);
                hlayout->addWidget(lbl);
                connect(delegate, &SpinBoxDelegate::indicate, [lbl] (bool state)
                {
                    QPixmap pix;
                    if (state == true)
                        pix = QIcon(":/pics/indicator_green.svg").pixmap(lbl->size());
                    else
                        pix = QIcon(":/pics/indicator_red.svg").pixmap(lbl->size());
                    lbl->setPixmap(pix);
                });
//                hlayout->addWidget(new QLabel("check", this));
            }

            hlayout->addWidget(table);
            vector.push_back(table);

            //DEBUG:
            model->setObjectName("model_" + tr("%1_%2").arg(i).arg(j));
            table->setObjectName("table_" + tr("%1_%2").arg(i).arg(j));
        }
        this->vecTables.push_back(vector);
    }


    // TableView c матрицами для альтернатив
    QScrollArea* scroll = new QScrollArea(ui->scrollAreaWidgetContents);
    scroll->setWidgetResizable(true);
    QWidget* widget = new QWidget();
    QHBoxLayout* hlayout = new QHBoxLayout(widget);
    scroll->setWidget(widget);
    mainLayout->addWidget(scroll);

    int tables = 1;
    for (int n : nums)
        tables *= n;

    vector.clear();


    auto llui = slist[slist.size() - 2];


    QStringList newList;

    for (int n = 0; n < llui.size(); n++) {
        for (int k = 0; k < llui[n].size(); ++k) {
            newList.push_back(llui[n][k]);
        }
    }



    for (int i = 0; i < tables; i++)
    {
        QTableView* table = new QTableView();
        QStandardItemModel* model = new QStandardItemModel(table);
        model->setRowCount(alternatives);
        model->setColumnCount(alternatives);
        table->setModel(model);
        //        table->setMinimumHeight(300);
        table->setMinimumWidth(300);
        SpinBoxDelegate* delegate =
                new SpinBoxDelegate(model->rowCount(), model->columnCount(), this);
        table->setItemDelegate(delegate);

        //BUG неверные индексы
            hlayout->addWidget(new QLabel(newList[i]));

        hlayout->addWidget(table);
        vector.push_back(table);
    }

    this->vecTables.push_back(vector);

    // TODO переписать
    mainLayout->setStretch(3, 20);
    mainLayout->setStretch(0, 11);
    mainLayout->setStretch(1, 16);
    mainLayout->setStretch(2, 16);



    QPushButton* calcButton = new QPushButton("Расчитать", this);
    mainLayout->addWidget(calcButton);
    //    calcButton->setChecked(false);
    calcButton->setDefault(true);
    connect(calcButton, &QPushButton::clicked, this, &Dialog::calculate);
}



Dialog::~Dialog()
{
    dumpObjectTree();
    qDebug() << "Dialog destroyed";
    delete ui;
}



// TODO дописать
void Dialog::setTitles(int prev, int level, const QList<QStringList>& list)
{
    //    this->slist.push_back(list);


    //    for (int i = 0; i < delim; ++i)
    for (int c = 0; c < vecTables[level].size(); ++c)
    {
        auto table = vecTables[level][c];
        QAbstractItemModel* model = table->model();
        QStandardItemModel* mod = dynamic_cast<QStandardItemModel*>(model);
        mod->setHorizontalHeaderLabels(list[c % prev]);
        mod->setVerticalHeaderLabels(list[c % prev]);
    }

}





void Dialog::defaultValue()
{
    for (QVector<QTableView*> vector : vecTables)
    {
        for (QTableView* table : vector)
        {
            SpinBoxDelegate* delegate = dynamic_cast<SpinBoxDelegate*>(table->itemDelegate());
            QAbstractItemModel* model = table->model();

            for (int col = 0; col < model->columnCount(); col++)
            {
                QModelIndex index = model->index(col, col);
                model->setData(index, 1);
                delegate->lockIndex(index);
                table->horizontalHeader()->resizeSection(col, 60);
                table->verticalHeader()->resizeSection(col, 25);
            }
        }
    }
}



void Dialog::calculate()
{
    qDebug() << "clicked CALCULATE";
    AlghorithmAHP ahp(this->alternatives);

    std::vector<std::vector<double>> CR;

    std::vector<Matrix> list;
    for (QVector<QTableView*> vector : vecTables)
    {
        list.clear();
        for (QTableView* table : vector)
        {
            QAbstractItemModel* model = table->model();
            Matrix mtx(model->rowCount(), model->columnCount());


            for (int row = 0; row < model->rowCount(); row++)
            {
                for (int col = 0; col < model->columnCount(); col++)
                {
                    QModelIndex index = model->index(row, col);
                    QVariant var = model->data(index);
                    qDebug() << var.toDouble();
                    mtx(row, col) = var.toDouble();
                }
            }
            list.push_back(mtx);
        }
        CR.push_back(ahp.addLevel(list));
    }


    for (uint i = 0; i < CR.size(); ++i) {
        for (uint j = 0; j < CR[i].size(); ++j) {
            auto check = CR[i][j];
            if (CR[i][j] > 0.1)
            {
                QMessageBox::critical(this, "Несогл", tr("матрица не согласованна\n") +
                                      "уровень: " + QVariant(i + 1).toString() + ", номер: " +
                                      QVariant(j + 1).toString() + "  " + QVariant(check).toString());
                return;
            }
        }
    }



    auto pair = ahp.answer();

    Dialog wgt(QVector<double>::fromStdVector(pair.second), this->slist.back().back(), pair.first, CR, this);
    wgt.defaultValue();

    wgt.setModal(true);
    wgt.setMinimumSize(300, 200);
    wgt.exec();
}



QLabel* Dialog::getLabel(const QString& file, const QRect& rect, QWidget* parent)
{
    QLabel* label = new QLabel(parent);
    label->setGeometry(rect);
    label->setAlignment(Qt::AlignCenter);
    label->setLineWidth(1);

    QPixmap pix = QIcon(file).pixmap(label->size());
    label->setPixmap(pix);
    return label;
}



void Dialog::setIndicator(bool state)
{

}
