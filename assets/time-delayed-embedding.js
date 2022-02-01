const update_inline_equations = function () {
  document.querySelectorAll(".dynamic-inline").forEach((eqn) => {
    const E = parseInt(document.getElementById("E").value);
    const tau = parseInt(document.getElementById("tau").value);

    eqn.innerHTML =
      "\\(" +
      eqn.dataset.equation.replace(/\${E}/, E).replace(/\${tau}/, tau) +
      "\\)";
  });
};

const update_centered_equations = function () {
  // Read in the manifold specifications from the sliders
  const numObs = parseInt(document.getElementById("numObs").value);
  const E = parseInt(document.getElementById("E").value);
  const tau = parseInt(document.getElementById("tau").value);
  const allowMissing = false;
  const p = 1;

  const x_time_series = latexify_time_series("x", numObs);

  // Construct the manifold and targets
  const M = manifold("x", numObs, E, tau, allowMissing, p);

  // Turn these into latex arrays
  const maniSetFormTex = latexify_set_of_sets(M.manifold);
  const maniTex = latexify(M.manifold);

  // Save the result to the page
  document.querySelectorAll(".dynamic-equation").forEach((eqn) => {
    eqn.innerHTML = eqn.dataset.equation
      .replace(/\${x_time_series}/, x_time_series)
      .replace(/\${M_x_sets}/, maniSetFormTex)
      .replace(/\${M_x}/, maniTex);
  });

  MathJax.typeset();
};

const sliders = document.querySelectorAll(".slider-container input");

sliders.forEach((slider) =>
  slider.addEventListener("input", function () {
    update_inline_equations();
    update_centered_equations();
  })
);

update_inline_equations();
update_centered_equations();
