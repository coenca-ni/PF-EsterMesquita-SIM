clc;
clear all;
% Carregar o arquivo CSV como uma tabela
T = readtable('SIM.csv');

% Converter a coluna 'created_at' para datetime, incluindo o fuso horário
T.created_at = datetime(T.created_at, 'InputFormat', 'yyyy-MM-dd''T''HH:mm:ssXXX', 'TimeZone', 'America/Sao_Paulo');

% Se desejar remover o fuso horário após a conversão
T.created_at.TimeZone = '';

%% Defina aqui as datas de início e fim para filtrar os dados
data_inicio = datetime(2024, 10, 01);  % Alterar para a data de início desejada
data_fim = datetime(2024, 10, 02);     % Alterar para a data de fim desejada

% Filtrar os dados dentro do intervalo de datas
idx = T.created_at >= data_inicio & T.created_at <= data_fim;
datas_filtradas = T.created_at(idx);
field7_filtrado = T.field7(idx);

% Remover valores NaN e valores anômalos (ex: fora de uma faixa de 0 a 100% para Qualidade de Transmissão)
mask = ~isnan(field7_filtrado) & field7_filtrado >= 0 & field7_filtrado <= 100;
datas_limpas = datas_filtradas(mask);
field7_limpas = field7_filtrado(mask);

% Parâmetros do filtro (tamanho da janela)
windowSize = 5;  % Ajuste conforme necessário

% 1. Filtro de Média Móvel
field7_media_movel = movmean(field7_limpas, windowSize);

% 2. Filtro Mediana
field7_mediana = medfilt1(field7_limpas, windowSize);

% Gerar os gráficos diretamente no layout atual (sem nova janela)

% Gráfico original (Laranja)
subplot(3,1,1);
plot(datas_limpas, field7_limpas, 'Color', [1, 0.5, 0]);  % Laranja
title('Dados Originais - Qualidade de Transmissão');
xlabel('Data e Hora');
ylabel('QT (%)');
grid on;

% Gráfico com Filtro de Média Móvel (Azul)
subplot(3,1,2);
plot(datas_limpas, field7_media_movel, 'Color', [0, 0.4470, 0.7410]);  % Azul
title('Filtro Média Móvel - Qualidade de Transmissão');
xlabel('Data e Hora');
ylabel('QT (%)');
grid on;

% Gráfico com Filtro Mediana (Verde)
subplot(3,1,3);
plot(datas_limpas, field7_mediana, 'Color', [0.4660, 0.6740, 0.1880]);  % Verde
title('Filtro Mediana - Qualidade de Transmissão');
xlabel('Data e Hora');
ylabel('QT (%)');
grid on;

% Ajustar o eixo x para exibir datas e horas em português
ax1 = subplot(3,1,1);
ax2 = subplot(3,1,2);
ax3 = subplot(3,1,3);

% Exibir data e hora no formato 'dd-MMM-yyyy HH:mm'
ax1.XAxis.TickLabelFormat = 'dd-MMM-yyyy HH:mm';  % Dia-Mês-Ano Hora:Minuto
ax2.XAxis.TickLabelFormat = 'dd-MMM-yyyy HH:mm';  % Dia-Mês-Ano Hora:Minuto
ax3.XAxis.TickLabelFormat = 'dd-MMM-yyyy HH:mm';  % Dia-Mês-Ano Hora:Minuto

% Alterar os nomes dos meses para português (de Oct para Out, etc.)
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Oct', 'Out');
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Sep', 'Set');
ax1.XTickLabel = strrep(ax1.XTickLabel, 'Jul', 'Jul');

ax2.XTickLabel = strrep(ax2.XTickLabel, 'Oct', 'Out');
ax2.XTickLabel = strrep(ax2.XTickLabel, 'Sep', 'Set');
ax2.XTickLabel = strrep(ax2.XTickLabel, 'Jul', 'Jul');

ax3.XTickLabel = strrep(ax3.XTickLabel, 'Oct', 'Out');
ax3.XTickLabel = strrep(ax3.XTickLabel, 'Sep', 'Set');
ax3.XTickLabel = strrep(ax3.XTickLabel, 'Jul', 'Jul');
