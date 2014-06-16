#include <QGuiApplication>
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QFileInfo>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>

static bool lint_file(const QString &filename, bool silent)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file" << filename << file.error();
        return false;
    }

    QByteArray code = file.readAll();
    file.close();
    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);
    lexer.setCode(code, /*line = */ 1);
    QQmlJS::Parser parser(&engine);

    QFileInfo info(filename);

    bool success = info.suffix().toLower() == QLatin1String("js") ? parser.parseProgram()
                                                                  : parser.parse();

    if (!success && !silent) {
        foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
            qWarning("%s:%d : %s", qPrintable(filename), m.loc.startLine, qPrintable(m.message));
        }
    }

    return success;
}

int main(int argv, char *argc[])
{
    QGuiApplication app(argv, argc);

    if (argv < 2) {
        qWarning() << "Usage: qmllint [--silent] <file1.qml>";
        return -2;
    }

    QStringList arguments = app.arguments();
    arguments.removeFirst();
    bool silent = arguments.removeAll(QString("--silent")) > 0;
    bool success = true;
    foreach (const QString &filename, arguments) {
        success &= lint_file(filename, silent);
    }
    
    return success ? 0 : -1;
}
