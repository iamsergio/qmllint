#include <QGuiApplication>
#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QFileInfo>
#include <QTextStream>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>

#if QT_VERSION >= 0x050300
# include <QtQml/private/qqmlirbuilder_p.h>
#else
# include <QtQml/private/qqmlscript_p.h>
#endif

static void remove_metadata(QString &code)
{
    // Removes .pragma library and such, otherwise we get a syntax error.
#if QT_VERSION >= 0x050300
        QQmlJS::DiagnosticMessage metaDataError;
        QmlIR::Document irUnit(/**debugger=*/false);
        irUnit.extractScriptMetaData(code, &metaDataError);
#else
        QQmlError metaDataError;
        QQmlScript::Parser::extractMetaData(code, &metaDataError);
#endif
}

static bool lint_file(const QString &filename, bool silent)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly)) {
        qWarning() << "Failed to open file" << filename << file.error();
        return false;
    }

    QString code = file.readAll();
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    bool isJavaScript = info.suffix().toLower() == QLatin1String("js");
    if (isJavaScript) {
        remove_metadata(/*by-ref*/code);
    }

    lexer.setCode(code, /*line = */ 1, true);
    QQmlJS::Parser parser(&engine);

    bool success = isJavaScript ? parser.parseProgram() : parser.parse();

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
        QTextStream stream(stdout);
        stream << QObject::tr("Usage: qmllint [--silent] <file1> ... <fileN>") << "\n\n"
               << QObject::tr("Where <file> is either a .js or a .qml file.") << "\n";
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
